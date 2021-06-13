#ifndef WEBSERVER_THREADS_H
#define WEBSERVER_THREADS_H


int init_threads(unsigned int threads_count);

void distribute_new_requests(void);

#endif //WEBSERVER_THREADS_H
