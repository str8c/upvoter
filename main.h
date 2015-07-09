#include <stdint.h>

enum {
    TEXT_HTML, IMAGE_PNG, DOWNLOAD,
};

typedef struct {
    void *data;
    void *cookie;
    void *redirect;
    uint32_t ip;
    uint8_t type;
    int cookie_len, redirect_len;
    char buf[1024 * 1024 * 2];
} pageinfo_t;
