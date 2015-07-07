#include <stdbool.h>
#include <stdint.h>

bool strcmp_slash(const char *a, const char *b);

uint32_t hash_user(const char *p);
uint32_t hash_sub(const char *p);
uint32_t hash_domain(const char *p);
uint32_t hash_domain_slash(const char *p);

int makelower(char *p, int len, const char *str);
const char* readlower(char *p, int len, const char *str);
