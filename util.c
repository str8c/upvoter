#include "util.h"

#include <stdio.h>
#include <stdlib.h>

bool strcmp_slash(const char *a, const char *b)
{
    do {
        if (*a != *b)
            return (!*a && *b == '/');
    } while (b++, *a++);
    return 0;
}

static char value(char ch)
{
    if (ch == '_')
        ch = 36;
    else if (ch >= 'A')
        ch -= 'A';
    else
        ch = (ch - '0') + 26;

    return ch;
}

static char value2(char ch)
{
    if (ch >= 'a')
        ch -= 'a';
    else
        ch = (ch - '0') + 26;

    return ch;
}

static char value3(char ch)
{
    if (ch >= 'a')
        ch -= 'a';
    else if (ch >= '0')
        ch = (ch - '0') + 26;
    else
        ch = 36;

    return ch;
}

uint32_t hash_user(const char *p)
{
    uint32_t v;
    int i;

    v = 0;
    i = 4;
    do {
        v *= 38;
        if (*p)
            v += value(*p++) + 1;
    } while (--i);

    return v;
}

uint32_t hash_sub(const char *p)
{
    uint32_t v;
    uint8_t value;
    int i;

    v = 0;
    i = 4;
    do {
        value = 0;
        if (*p) {
            if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')))
                return 0;

            value = value2(*p++) + 1;
        }

        if (i) {
            v *= 38;
            v += value;
            i--;
        }

    } while (i || *p);

    return v;
}

uint32_t hash_domain(const char *p)
{
    uint32_t v;
    int i;

    v = 0;
    i = 4;
    do {
        v *= 38;
        if (*p)
            v += value3(*p++) + 1;
    } while (--i);

    return v;
}

uint32_t hash_domain_slash(const char *p)
{
    uint32_t v;
    int i;

    v = 0;
    i = 4;
    do {
        v *= 38;
        if (*p != '/')
            v += value3(*p++) + 1;
    } while (--i);

    return v;
}

int makelower(char *p, int len, const char *str)
{
    const char *s;
    char ch;

    s = str;
    do {
        ch = 0;
        if (*s) {
            ch = *s++;
            if (ch >= 'a')
                ch -= ('a' - 'A');
        }
        *p++ = ch;
    } while (--len);

    return (s - str);
}

const char* readlower(char *p, int len, const char *str)
{
    char ch;

    do {
        ch = 0;
        if (*str && *str != '/') {
            ch = *str++;
            if (ch >= 'a' && ch <= 'z')
                ch -= ('a' - 'A');
            else if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_'))
                return 0;
        }
        *p++ = ch;
    } while (--len);

    if (*str++ != '/')
        return 0;

    return str;
}
