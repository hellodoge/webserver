#include <string.h>

#define MIME_TYPE_DEFAULT "text/plain"


typedef struct {
    char *ext;
    char *type;
} mime_t;

const mime_t mime_types[] = {
        {"htm",  "text/html"},
        {"html", "text/html"},
        {"css",  "text/css"},
        {"js",   "text/javascript"},
        {"ico",  "image/x-icon"},
        {"jpg",  "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"png",  "image/png"},
        {"gif",  "image/gif"}
};


const char *find_mime_type(const char *file) {
    char *ext = strrchr(file, '.');
    if (ext == NULL)
        return MIME_TYPE_DEFAULT;
    for (size_t i = 0; i < sizeof(mime_types) / sizeof(mime_t); i++) {
        if (!strncmp(ext + 1, mime_types[i].ext, strlen(mime_types[i].ext)))
            return mime_types[i].type;
    }
    return MIME_TYPE_DEFAULT;
}
