
#include "include/connection.h"
#include "include/response.h"
#include "include/util.h"

#include <stdnoreturn.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <strings.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define CONNECTION_TIMEOUT 120 /* seconds */

#define SIG_NEW_CONNECTION SIGUSR1
#define MAX_CONNECTIONS_PER_THREAD FD_SETSIZE


typedef struct {
    pthread_t thread;
    connection_queue_t active;
    unsigned int _Atomic active_connections_count;
    bool initialized;
} server_thread_t;


server_thread_t *thread_poll;
unsigned int threads_count;

/// initialized in init_threads
sigset_t original_signal_mask, mask_sig_new_connection_blocked;

void add_new_connections(server_thread_t *thread);

void serve_active(server_thread_t *thread);

noreturn void *thread_run(void *thread) {
    pthread_sigmask(SIG_SETMASK, &mask_sig_new_connection_blocked, NULL);
    for (;;) {
        add_new_connections(thread);
        serve_active(thread);
    }
}

/// should be called after appending new request
void distribute_new_requests(void) {
    unsigned int min_active_connections = thread_poll[0].active_connections_count;
    pthread_t *chosen_thread = &thread_poll[0].thread;
    for (size_t i = 1; i < threads_count; i++) {
        if (min_active_connections > thread_poll[i].active_connections_count) {
            min_active_connections = thread_poll[i].active_connections_count;
            chosen_thread = &thread_poll[i].thread;
        }
    }
    if (min_active_connections == 0 || min_active_connections >= MAX_CONNECTIONS_PER_THREAD)
        return;
    pthread_kill(*chosen_thread, SIG_NEW_CONNECTION);
}

static inline size_t get_thread_number(const server_thread_t *thread) {
    return thread - thread_poll;
}

void destruct_connection(connection_t *connection);

void serve_active(server_thread_t *thread) {
    fd_set waiting_for_data;
    FD_ZERO(&waiting_for_data);
    int highest_fd = 0;
    time_t first_terminated_after = CONNECTION_TIMEOUT;
    {
        time_t current_time;
        time(&current_time);
        for (connection_t *connection = thread->active.first, *next;
             connection != NULL; connection = next) {
            next = connection->next;
            if (connection->added_to_active_at > current_time + CONNECTION_TIMEOUT) {
                thread->active_connections_count--;
                remove_connection(&thread->active, connection);
                destruct_connection(connection);
                continue;
            }
            FD_SET(connection->fd, &waiting_for_data);
            if (highest_fd < connection->fd)
                highest_fd = connection->fd;
            if (first_terminated_after > CONNECTION_TIMEOUT - (current_time - connection->added_to_active_at))
                first_terminated_after = CONNECTION_TIMEOUT - (current_time - connection->added_to_active_at);
        }
    }

    if (thread->active_connections_count == 0)
        return;

    struct timespec wake_after = {.tv_sec = first_terminated_after};

    int select_result = pselect(highest_fd + 1, &waiting_for_data, NULL,
                                NULL, &wake_after, &original_signal_mask);
    if (select_result <= 0) {
        if (select_result < 0 && errno != EINTR)
            perror("select");
        return;
    }
    for (connection_t *connection = thread->active.first, *next;
         connection != NULL; connection = next) {
        next = connection->next;
        if (FD_ISSET(connection->fd, &waiting_for_data)) {
            parser_state_t parsing_state = parse_http_request(connection->fd, &connection->request,
                                                              &connection->state,
                                                              (struct parser_context **) &connection->context);
            if (parsing_state == PARSE_WAITING) {
                continue; //next connection
            } else {
                remove_connection(&thread->active, connection);
                http_request_t *request = connection->request;
                if (parsing_state == PARSE_OK) {
                    http_response_t response = process_request(request);
                    send_response(connection->fd, &response);
                    access_log(get_thread_number(thread), request, &response);
                    deallocate_response_fields(&response);
                }
                destruct_connection(connection);
                thread->active_connections_count--;
            }
        }
    }
}

void add_new_connections(server_thread_t *thread) {
    for (;;) {
        if (thread->active_connections_count > 0) {
            if (thread->active_connections_count >= MAX_CONNECTIONS_PER_THREAD)
                return;
            if (sem_trywait(&new_connections) == -1)
                return;
        } else
            sem_wait(&new_connections);
        pthread_mutex_lock(&connections_queue_waiting_mutex);
        connection_t *new_connection = pop_connection(&connections_queue_waiting);
        pthread_mutex_unlock(&connections_queue_waiting_mutex);
        append_connection(&thread->active, new_connection);
        thread->active_connections_count++;
        new_connection->added_to_active_at = time(NULL);
    }
}

void destruct_threads(void);

void handle_new_connection_signal() {}

/// must only be called after call of init_new_connections defined in connection.h
int init_threads(const unsigned int count) {
    signal(SIG_NEW_CONNECTION, handle_new_connection_signal);
    sigprocmask(0 /*ignored because second argument is NULL*/, NULL, &original_signal_mask);
    sigemptyset(&mask_sig_new_connection_blocked);
    sigaddset(&mask_sig_new_connection_blocked, SIG_NEW_CONNECTION);
    threads_count = count;
    thread_poll = malloc(sizeof(*thread_poll) * count);
    bzero(thread_poll, sizeof(*thread_poll) * count);
    atexit(destruct_threads);
    for (size_t i = 0; i < count; i++) {
        thread_poll[i].active_connections_count = ATOMIC_VAR_INIT(0);
        if (pthread_create(&thread_poll[i].thread, NULL,
                           thread_run, &thread_poll[i].thread) != 0) {
            return -1;
        }
        thread_poll[i].initialized = true;
    }
    return 0;
}


void destruct_threads(void) {
    for (size_t i = 0; i < threads_count; i++) {
        if (thread_poll[i].initialized == false)
            break;
        pthread_kill(thread_poll[i].thread, SIGKILL);
        if (pthread_join(thread_poll[i].thread, NULL) != 0) {
            perror("terminate threads");
        }
        connection_t *connection = thread_poll[i].active.first;
        while (connection != NULL) {
            close(connection->fd);
            deallocate_request_fields(connection->request);
            if (connection->context != NULL)
                free(connection->context);
            connection = connection->next;
        }
    }
    free(thread_poll);
    thread_poll = NULL;
    threads_count = 0;
}

void destruct_connection(connection_t *connection) {
    deallocate_request_fields(connection->request);
    free(connection->request);
    close(connection->fd);
    free(connection);
}