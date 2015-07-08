#include <stdbool.h>
#include <stdint.h>

bool strcmp_slash(const char *a, const char *b);

uint32_t hash_user(const char *p);
uint32_t hash_sub(const char *p);
uint32_t hash_domain(const char *p);
uint32_t hash_domain_slash(const char *p);

uint32_t makelower(char *p, uint32_t len, const char *str);
const char* readlower(char *p, uint32_t len, const char *str);


enum {
    decode_raw,
    decode_comment,
    decode_url,
    decode_title,
};

const char* text_decode(const char *p, char **textp, int level);
