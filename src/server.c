#include "include/parser.h"
#include "include/threads.h"

#include <stdnoreturn.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>

noreturn void runserver(const int listen_fd) {
    init_new_connections();
    if (init_threads() == -1) {
        exit(EXIT_FAILURE);
    }
    for (;;) {
        struct sockaddr_in conn_addr;
        int connfd = accept(listen_fd, (struct sockaddr *) &conn_addr, &(socklen_t){sizeof(conn_addr)});
        if (connfd < 0) {
            perror("Error accepting request");
            continue;
        }

        connection_t *connection = init_connection(connfd);
        pthread_mutex_lock(&connections_queue_waiting_mutex);
        append_connection(&connections_queue_waiting, connection);
        pthread_mutex_unlock(&connections_queue_waiting_mutex);
        sem_post(&new_connections);
        distribute_new_requests();
    }
}

