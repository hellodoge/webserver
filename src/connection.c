#include "include/connection.h"

#include <unistd.h>
#include <pthread.h>
#include <strings.h>
#include <semaphore.h>
#include <stdnoreturn.h>
#include <fcntl.h>


connection_queue_t connections_queue_waiting = {
        .first = NULL,
        .last = NULL,
};

noreturn void *thread_run(void *thread_void_p);

pthread_mutex_t connections_queue_waiting_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t new_connections;

#define NOT_SHARED_BETWEEN_PROCESSES 0

connection_t *init_connection(int fd) {
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | O_NONBLOCK);
    connection_t *connection = malloc(sizeof(connection_t));
    bzero(connection, sizeof(*connection));
    connection->state = CS_NOT_INITIALIZED;
    connection->fd = fd;
    return connection;
}

void destruct_new_connections(void);

void init_new_connections(void) {
    const unsigned int initial_semaphore_value = 0;
    sem_init(&new_connections, NOT_SHARED_BETWEEN_PROCESSES, initial_semaphore_value);
    atexit(destruct_new_connections);
}

#undef NOT_SHARED_BETWEEN_PROCESSES //init_new_connections

connection_t *append_connection(connection_queue_t *queue, connection_t *connection) {
    if (queue->last != NULL) {
        connection->prev = queue->last;
        queue->last->next = connection;
        queue->last = connection;
    } else {
        queue->first = connection;
        queue->last = connection;
    }
    return connection;
}

connection_t *remove_connection(connection_queue_t *queue, connection_t *connection) {
    if (connection->prev != NULL)
        connection->prev->next = connection->next;
    if (connection->next != NULL)
        connection->next->prev = connection->prev;
    if (queue->first == connection)
        queue->first = connection->next;
    if (queue->last == connection)
        queue->last = connection->prev;
    return connection;
}

connection_t *pop_connection(connection_queue_t *queue) {
    if (queue->first == NULL) {
        return NULL;
    } else {
        connection_t *connection = queue->first;
        queue->first = connection->next;
        if (queue->last == connection)
            queue->last = NULL;
        connection->prev = NULL;
        connection->next = NULL;
        return connection;
    }
}

void destruct_new_connections(void) {
    sem_destroy(&new_connections);
    pthread_mutex_lock(&connections_queue_waiting_mutex);
    connection_t *connection;
    while ((connection = pop_connection(&connections_queue_waiting)) != NULL) {
        close(connection->fd);
        free(connection);
    }
    pthread_mutex_unlock(&connections_queue_waiting_mutex);
}