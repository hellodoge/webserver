#include "include/listener.h"
#include "include/server.h"
#include "include/static.h"
#include "include/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#define DEFAULT_PORT 8080


void print_help_message_and_exit(const char *executable) {
    printf("usage: %s -p <port> -r <website root directory>", executable);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;
    char *path = NULL;

    const char *executable = argv[0];
    int current_opt;
    while ((current_opt = getopt(argc, argv, "p:r:")) != -1) {
        switch (current_opt) {
            case 'p':
                errno = 0; // can be set to non zero value by strtol
                port = (int) strtol(optarg, NULL, 10);
                if (errno != 0)
                    print_help_message_and_exit(executable);
                break;
            case 'r':
                path = optarg;
                break;
            case '?':
                print_help_message_and_exit(executable);
            default:
                abort();
        }
    }

    if (path == NULL)
        print_help_message_and_exit(executable);

	int fd = init_listener(port);
	if (fd < 0) {
		perror("Server initialization failed");
		return EXIT_FAILURE;
	}

	handle_signals();

	if (init_root(path) != 0) {
		fprintf(stderr, "Invalid path: %s\n", path);
	}

	printf("Starting serving %s at http://127.0.0.1:%d\n", get_root(), port);
	runserver(fd);
}