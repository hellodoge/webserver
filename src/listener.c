#include "include/listener.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>

#define ENABLE 1


int listen_fd = -1;

void shutdown_server(void);

/// security: do not call twice
/// return value: returns listener file descriptor if successful, otherwise -1
int init_listener(uint16_t port) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        return -1;

    /// prevent "port in use" error
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ENABLE}, sizeof(int));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    bzero(address.sin_zero, sizeof(address.sin_zero));

    if (bind(listen_fd, (const struct sockaddr *) &address, sizeof(address)) != 0)
        return -1;

    atexit(shutdown_server);

    if (listen(listen_fd, MAX_QUERY) != 0)
        return -1;

    return listen_fd;
}

void shutdown_server(void) {
    shutdown(listen_fd, SHUT_RDWR);
    close(listen_fd);
}