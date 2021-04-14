#ifndef WEBSERVER_PARSER_H
#define WEBSERVER_PARSER_H

#include "connection.h"
#include "request.h"


typedef enum {
    PARSE_OK,
    PARSE_ERROR,
    PARSE_WAITING,
} parser_state_t;

struct parser_context;

parser_state_t parse_http_request(int fd, http_request_t **request, connection_state_t *state,
                                  struct parser_context **context);

#endif //WEBSERVER_PARSER_H
