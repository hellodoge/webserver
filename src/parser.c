#include "include/parser.h"

#include "include/util.h"
#include "include/connection.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <assert.h>


struct parser_context {
    char *buffer;
    char *cursor;
    header_t *header; // used by read_header_line
    long int content_length; // used by waiting for content (parse_http_request)
};

typedef enum {
    RS_OK,
    RS_WAITING,
    RS_BUFFER_OVERFLOW,
    RS_TOKEN_NOT_FOUND,
} read_state_t;

read_state_t read_token(int fd, const char *delim, char *buffer, size_t buffer_size,
                        struct parser_context **context);

read_state_t read_header_line(int fd, struct parser_context **context, header_t **header);

#define READ_REQUEST_LINE_TOKEN(field, delim) { \
    if (*context == NULL || (*context)->buffer == field) { \
        read_state_t parse_state = read_token(fd, delim, field, \
                                               sizeof(field), context); \
        if (parse_state == RS_WAITING) { \
            *state = CS_WAITING_FOR_REQUEST_LINE; \
            return 0; \
        } else if (parse_state != RS_OK) { \
            if (*context != NULL) { \
                free(*context); \
                *context = NULL; \
            } \
            return -1; \
        } \
    } \
}


parser_state_t parse_http_request(int fd, http_request_t **request, connection_state_t *state,
                                  struct parser_context **context) {
    assert(state != NULL && request != NULL && context != NULL);

    if (*state == CS_NOT_INITIALIZED) {
        *request = malloc(sizeof(**request));
        bzero(*request, sizeof(**request));
        *state = CS_WAITING_FOR_REQUEST_LINE;
    }

    if (*state == CS_WAITING_FOR_REQUEST_LINE) {
        READ_REQUEST_LINE_TOKEN((*request)->method, " ")
        READ_REQUEST_LINE_TOKEN((*request)->path, " ")
        READ_REQUEST_LINE_TOKEN((*request)->version, NULL)
        *state = CS_WAITING_FOR_HEADER;
    }

    if (*state == CS_WAITING_FOR_HEADER) {
        header_t *header;
        read_state_t code;
        while ((code = read_header_line(fd, context, &header)) == RS_OK) {
            if (header_insert((*request)->header, header) == -1) {
                free(header);
                if (*context != NULL) {
                    free(*context);
                    *context = NULL;
                }
                return PARSE_ERROR;
            }
        }
        if (code == RS_WAITING) {
            return PARSE_WAITING;
        } else if (code != RS_TOKEN_NOT_FOUND) {
            if (*context != NULL) {
                free(*context);
                *context = NULL;
            }
            return PARSE_ERROR;
        }
        *state = CS_WAITING_FOR_CONTENT;
    }

    if (*state == CS_WAITING_FOR_CONTENT) {
        long int content_length = 0;
        if (*context == NULL) {
            const char *header_content_length = header_get((*request)->header, "content-length");
            if (header_content_length != NULL) {
                content_length = strtol(header_content_length, NULL, 10);
                if (content_length < 0) {
                    return -1;
                }
            }
        } else {
            content_length = (*context)->content_length;
        }
        if (content_length > 0) {
            long int content_read;
            void *buffer;
            if (*context == NULL) {
                buffer = malloc(content_length);
                if (buffer == NULL)
                    return -1;
                content_read = 0;
            } else {
                buffer = (*context)->buffer;
                content_read = (*context)->cursor - (*context)->buffer;
            }
            while (content_read < content_length) {
                long int block_read = read(fd, buffer + content_read, content_length - content_read);
                if (block_read == 0) {
                    *context = malloc(sizeof(**context));
                    (*context)->buffer = buffer;
                    (*context)->cursor = buffer + content_read;
                    (*context)->content_length = content_length;
                    return PARSE_WAITING;
                }
                content_read += block_read;
            }
            (*request)->body = buffer;
            (*request)->body_length = content_length;
        }
    }

    *state = CS_PARSED;

    shutdown(fd, SHUT_RD);
    return PARSE_OK;
}

#undef READ_REQUEST_LINE_TOKEN

read_state_t read_token(int fd, const char *delim, char *buffer, size_t buffer_size,
                        struct parser_context **context) {
    assert(context != NULL);

    char *cursor = buffer;
    if ((*context) != NULL) {
        buffer = (*context)->buffer;
        cursor = (*context)->cursor;
    }
    char current;
    while (read(fd, &current, 1) == 1) {
        if (current == '\r' || (buffer == cursor && (current == ' ' || current == '\n'))) {
            if (buffer == cursor && current == '\n') {
                return RS_TOKEN_NOT_FOUND;
            } else {
                continue;
            }
        } else if (current == '\n' || (delim != NULL && strchr(delim, current) != NULL)) {
            *cursor = '\0';
            if (*context != NULL) {
                free(context);
                *context = NULL;
            }
            return RS_OK;
        }
        *cursor++ = current;
        if (cursor >= &buffer[buffer_size] - 1) {
            return RS_BUFFER_OVERFLOW;
        }
    }

    if ((*context) == NULL) {
        (*context) = malloc(sizeof(**context));
    }
    (*context)->buffer = buffer;
    (*context)->cursor = cursor;

    return RS_WAITING;
}

#define READ_HEADER_LINE(field, delim) { \
    if (*context == NULL || (*context)->buffer == field) { \
        read_state_t parse_state = read_token(fd, delim, field, \
                                               sizeof(field), context); \
        if (parse_state == RS_WAITING) { \
            (*context)->header = *header; \
            return RS_WAITING; \
        } else if (parse_state != RS_OK) { \
            free(*header); \
            *header = NULL; \
            return parse_state; \
        } \
    } \
}

read_state_t read_header_line(int fd, struct parser_context **context, header_t **header) {
    assert(context != NULL && header != NULL);

    if ((*context) == NULL) {
        *header = malloc(sizeof(header_t));
    } else {
        assert((*context)->header != NULL);
        *header = (*context)->header;
    }
    READ_HEADER_LINE((*header)->key, ":")
    READ_HEADER_LINE((*header)->value, NULL)

    if (*context != NULL) {
        free(*context);
        *context = NULL;
    }

    return RS_OK;
}

#undef READ_HEADER_LINE