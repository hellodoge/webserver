#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <stdnoreturn.h>

noreturn void runserver(int listen_fd, unsigned int threads_count);

#endif //WEBSERVER_SERVER_H
