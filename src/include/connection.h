#ifndef WEBSERVER_CONNECTION_H
#define WEBSERVER_CONNECTION_H

#include "request.h"

#include <time.h>
#include <semaphore.h>
#include <signal.h>

typedef enum {
    CS_NOT_INITIALIZED,
    CS_WAITING_FOR_REQUEST_LINE,
    CS_WAITING_FOR_HEADER,
    CS_WAITING_FOR_CONTENT,
    CS_PARSED,
} connection_state_t;


typedef struct connection {
    int fd;
    connection_state_t state;
    http_request_t *request;
    void *context;
    time_t added_to_active_at;
    struct connection *next;
    struct connection *prev;
} connection_t;

typedef struct {
    connection_t *first;
    connection_t *last;
} connection_queue_t;

extern sem_t new_connections;

extern pthread_mutex_t connections_queue_waiting_mutex;

extern connection_queue_t connections_queue_waiting;

connection_t *init_connection(int fd);

connection_t *append_connection(connection_queue_t *queue, connection_t *connection);

connection_t *remove_connection(connection_queue_t *queue, connection_t *connection);

connection_t *pop_connection(connection_queue_t *queue);

void init_new_connections(void);

#endif //WEBSERVER_CONNECTION_H
