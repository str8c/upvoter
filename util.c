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

uint32_t makelower(char *p, uint32_t len, const char *str)
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

const char* readlower(char *p, uint32_t len, const char *str)
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

static int tohex(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';

    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;

    return -1;
}

static bool iswhitespace(char ch)
{
    return (ch == ' ' || ch == '\t');
}

static bool isignore(char ch)
{
    return (ch == '\r');
}

const char* text_decode(const char *p, char **textp, int level)
{
    char ch;
    int h1, h2;
    char *tp, *lp;
    bool prev_lb, prev_ws;

    tp = *textp;

    if (*p != '=' || !*p)
        return 0;

    p++;

    if (*p == '&' || !*p)
        return 0;

    lp = tp;
    ch = *p++;
    prev_lb = 1;
    prev_ws = 1;
    do {
        if(ch == '+') {
            ch = ' ';
        } else if(ch == '%') {
            if((h1 = tohex(p[0])) >= 0 && (h2 = tohex(p[1])) >= 0) {
                p += 2;
                ch = (h1 << 4) | h2;
                if(ch == '&') {
                    *tp++ = '&';
                    *tp++ = 'a';
                    *tp++ = 'm';
                    *tp++ = 'p';
                    ch = ';';
                }
            }
        }

        if(ch == '<' || ch == '>') {
            *tp++ = '&';
            *tp++ = (ch == '<') ? 'l' : 'g';
            *tp++ = 't';
            ch = ';';
        }

        if (ch == '"' && level == 2)
            return 0;

        if (ch == '\n') {
            if (level & 2)
                return 0;

            if ((level & 1) && prev_lb)
                continue;

            prev_lb = 1;
            *tp++ = '<';
            *tp++ = 'b';
            *tp++ = 'r';
            *tp++ = '>';
            continue;
        } else if (iswhitespace(ch)) {
            if (level) {
                if (!(level & 1))
                    return 0;

                if (prev_ws)
                    continue;

                prev_ws = 1;
            }
        } else if (isignore(ch)) {
            continue;
        } else {
            prev_lb = 0;
            prev_ws = 0;
            lp = tp + 1;
        }
        *tp++ = ch;
    } while(*p && (ch = *p++) != '&');

    *lp++ = 0;
    *textp = lp;
    return p;
}
