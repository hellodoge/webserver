#ifndef WEBSERVER_UTIL_H
#define WEBSERVER_UTIL_H


#include "parser.h"
#include "response.h"

int handle_signals(void);

void access_log(size_t thread_number, http_request_t *request, http_response_t *response);

int header_insert(header_t *buffer[], header_t *header);

const char *header_get(header_t *buffer[], const char *key);

void deallocate_request_fields(http_request_t *request);


#endif //WEBSERVER_UTIL_H
