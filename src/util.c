#include "include/parser.h"
#include "include/response.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>


#define TIME_BUFFER_LEN 128

void access_log(size_t thread_number, http_request_t *request, http_response_t *response) {
    time_t current = time(NULL);
    struct tm *local = localtime(&current);
    char buffer[TIME_BUFFER_LEN];
    strftime(buffer, TIME_BUFFER_LEN, "%d %b %X", local);
    printf("thread %ld [%s]: %s %s %s > %d %s\n", thread_number, buffer, request->method,
            request->path, request->version, response->status, response->message);
    fflush(stdout);
}

#undef TIME_BUFFER_LEN // access_log

void signal_handler() {
    exit(0);
}

/// return value: returns 0 if successful, otherwise -1
int handle_signals(void) {
    if (signal(SIGTERM, signal_handler) == SIG_ERR ||
        signal(SIGINT, signal_handler) == SIG_ERR ||
        signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return -1;
    }
    return 0;
}

// http://www.cse.yorku.ca/~oz/hash.html
unsigned long djb2_hash(const unsigned char *str) {
    unsigned long hash = 5381;
    unsigned char c;

    while ((c = *str++) != '\0')
        hash = ((hash << 5) + hash) + c;

    return hash;
}

char *lower_string(char *str) {
    for (char *p = str; *p != '\0'; p++)
        *p = (char)tolower(*p);
    return str;
}

/// return value: on success returns 0, otherwise -1
int header_insert(header_t *buffer[], header_t *header) {
    lower_string(header->key);
    const size_t position = djb2_hash((unsigned char *)header->key) % HEADER_BUFFER_SLOTS;
    for (size_t i = 0; i < HEADER_BUFFER_SLOTS; i++) {
        size_t lookup = (position + i) % HEADER_BUFFER_SLOTS;
        if (buffer[lookup] == NULL) {
            buffer[lookup] = header;
            return 0;
        }
    }
    return -1;
}

/// key argument must be lowercase
const char *header_get(header_t *buffer[], const char *key) {
    const size_t position = djb2_hash((unsigned char *)key) % HEADER_BUFFER_SLOTS;
    for (size_t i = 0; i < HEADER_BUFFER_SLOTS; i++) {
        size_t lookup = (position + i) % HEADER_BUFFER_SLOTS;
        if (buffer[lookup] == NULL) {
            break;
        } else {
            if (strcmp(buffer[lookup]->key, key) == 0)
                return buffer[lookup]->value;
        }
    }
    return NULL;
}

void deallocate_request_fields(http_request_t *request) {
    for (size_t i = 0; i < HEADER_BUFFER_SLOTS; i++) {
        if (request->header[i] != NULL)
            free(request->header[i]);
    }
    if (request->body != NULL)
        free(request->body);
}

