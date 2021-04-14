#ifndef WEBSERVER_STATIC_H
#define WEBSERVER_STATIC_H


#define PAGE404_PATH "/404.html"


int init_root(const char *relative_path);

char *find_static(const char *url_path);

const char *get_root(void);

#endif //WEBSERVER_STATIC_H
