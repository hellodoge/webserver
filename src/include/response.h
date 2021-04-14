#ifndef WEBSERVER_RESPONSE_H
#define WEBSERVER_RESPONSE_H

#include <stdlib.h>
#include "parser.h"


#define HTTP_VERSION_USING "HTTP/1.1"

typedef struct {
    int status;
    const char *message;
    const char *mime_type;
    ssize_t length;
    char *file;
    void *content;
} http_response_t;


http_response_t process_request(const http_request_t *request);

void send_response(int fd, const http_response_t *response);

void deallocate_response_fields(http_response_t *response);

#endif //WEBSERVER_RESPONSE_H
