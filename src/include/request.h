#ifndef WEBSERVER_REQUEST_H
#define WEBSERVER_REQUEST_H

#include <linux/limits.h>
#include <stdlib.h>

#define METHOD_MAX 32
#define VERSION_MAX 32
#define HEADER_KEY_MAX 128
#define HEADER_VALUE_MAX 512
#define HEADER_BUFFER_SLOTS 128

typedef struct {
    char key[HEADER_KEY_MAX];
    char value[HEADER_VALUE_MAX];
} header_t;

typedef struct {
    char method[METHOD_MAX];
    char path[PATH_MAX];
    char version[VERSION_MAX];
    header_t *header[HEADER_BUFFER_SLOTS];
    void *body;
    size_t body_length;
} http_request_t;

#endif //WEBSERVER_REQUEST_H
