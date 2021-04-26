#include "include/response.h"

#include "include/parser.h"
#include "include/static.h"
#include "include/mime.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define GENERIC_STATUS_CONTENT_BUFFER_SIZE 1024
#define DEFAULT_BUFFER_SIZE 4096

void handle_get_request(const http_request_t *request, http_response_t *buffer);


http_response_t process_request(const http_request_t *request) {
    http_response_t response = {
            .status = 500,
            .message = "Server Error",
            .mime_type = NULL,
            .length = -1,
            .file = NULL,
            .content = NULL
    };

    if (strcmp(request->method, "GET") == 0) {
        handle_get_request(request, &response);
    } else {
        response.status = 501;
        response.message = "Not Implemented";
    }

    if (response.file == NULL) {
        char buffer[PATH_MAX];
        sprintf(buffer, "/%d.html", response.status);
        response.file = find_static(buffer);
    }

    if (response.file != NULL) {
        struct stat st;
        stat(response.file, &st);
        response.length = st.st_size;
        response.mime_type = find_mime_type(response.file);
    } else {
        response.mime_type = "text/html";
        response.content = malloc(GENERIC_STATUS_CONTENT_BUFFER_SIZE);
        response.length = snprintf(response.content, GENERIC_STATUS_CONTENT_BUFFER_SIZE,
                 "<h1>%d</h1>%s",
                 response.status, response.message);
    }

    return response;
}

void handle_get_request(const http_request_t *request, http_response_t *buffer) {
    buffer->file = find_static(request->path);
    if (buffer->file != NULL) {
        buffer->status = 200;
        buffer->message = "OK";
    } else {
        buffer->status = 404;
        buffer->message = "Not Found";
    }
}

int send_file(int fd, const char *path);

void send_response(const int fd, const http_response_t *response) {
    dprintf(fd, "%s %d %s\n", HTTP_VERSION_USING, response->status, response->message);
    if (response->mime_type != NULL)
        dprintf(fd, "Content-Type: %s\n", response->mime_type);
    if (response->length >= 0)
        dprintf(fd, "Content-Length: %ld\n", response->length);
    write(fd, "\n", 1); // end of header
    if (response->content != NULL) {
        assert(response->length >= 0);
        write(fd, response->content, response->length);
    }
    if (response->file != NULL) {
        send_file(fd, response->file);
    }
    shutdown(fd, SHUT_WR);
}


int send_file(const int fd, const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return -1;

    char buffer[DEFAULT_BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, sizeof(char), DEFAULT_BUFFER_SIZE, file)) > 0)
        write(fd, buffer, bytes_read);
    return 0;
}

void deallocate_response_fields(http_response_t *response) {
    if (response->content != NULL)
        free(response->content);
    if (response->file != NULL)
        free(response->file);
}