#include <stdbool.h>
#include <stdint.h>

uint32_t current_time;

int find_str(const char *p, const char **str);

bool strcmp_slash(const char *a, const char *b);

uint32_t hash_user(const char *p);
uint32_t hash_sub(const char **s, int len);
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

const char* text_decode(const char *p, char **textp, int maxlength, int level);
const char* text_decode_markup(const char *p, char **textp, int maxlength);
const char* text_decode_name(const char *p, char *res, int res_len);

const char* read_id(const char *p, uint32_t *id, char end);
char* print_id(char *p, uint32_t id); /* writes up to 8 bytes in p */

char* print_time(char *p, uint32_t time, uint32_t edit_time); /* writes up to 15 bytes in p */
