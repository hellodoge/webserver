#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ROOT_FILE "/index.html"


char root_dir[PATH_MAX];
int root_dir_path_len;


/// return value: on success returns 0, otherwise -1
int init_root(const char *relative_path) {
    if (realpath(relative_path, root_dir) == NULL)
        return -1;
    root_dir_path_len = (int) strlen(root_dir);
    return 0;
}

/// return value should be deallocated
char *find_static(const char *url_path) {

    if (*url_path == '/' && (*(url_path + 1) == '\0' || *(url_path + 1) == '?'))
        url_path = ROOT_FILE;

    char relative_path[PATH_MAX];
    strcpy(relative_path, root_dir);
    char *cursor = relative_path + root_dir_path_len;
    const char *pointer = url_path;
    if (*pointer != '/') {
        return NULL; // TODO
    }
    while (*pointer != '\0') {
        if (*pointer == '%') {
            char code[3];
            strncpy(code, pointer + 1, 2);
            code[2] = '\0';
            *cursor++ = (char) strtol(code, NULL, 16);
            pointer += 3;
        } else if (*pointer == '?' || *pointer == '#') {
            break;
        } else {
            *cursor++ = *pointer++;
        }
        if (cursor >= &relative_path[PATH_MAX] - 1) {
            return NULL;
        }
    }
    *cursor = '\0';
    char *absolute_path = malloc(PATH_MAX);
    struct stat st;
    if ((realpath(relative_path, absolute_path) == NULL) ||
        strncmp(absolute_path, root_dir, root_dir_path_len) != 0 ||
        (stat(absolute_path, &st) != 0) ||
        (!S_ISREG(st.st_mode))) {

        free(absolute_path);
        return NULL;
    }
    return absolute_path;
}

/// safety: only call after a successful init_root call
const char *get_root(void) {
    return root_dir;
}