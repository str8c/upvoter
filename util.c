#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int find_str(const char *p, const char **str)
{
    int i;

    i = 0;
    do {
        if (!strcmp(p, *str++))
            return i;
        i++;
    } while (*str);
    return -1;
}

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

uint32_t hash_sub(const char **s, int len)
{
    const char *p;
    uint32_t v;
    uint8_t value;
    int i;

    p = *s;
    v = 0;
    i = 4;
    do {
        value = 0;
        if (*p != '/') {
            if (!--len)
                return 0;

            if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')))
                return 0;

            value = value2(*p++) + 1;
        } else if (i == 3) { /* dont allow 1 letter sub names */
            return 0;
        }

        if (i) {
            v *= 38;
            v += value;
            i--;
        }

    } while (i || *p != '/');

    *s = p + 1;
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

const char* text_decode(const char *p, char **textp, int maxlength, int level)
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

        if (ch == '"') {
            *tp++ = '&';
            *tp++ = 'q';
            *tp++ = 'u';
            *tp++ = 'o';
            *tp++ = 't';
            ch = ';';
        }

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
        if (!--maxlength) {
            if (!level)
                return 0;

            while (*p && *p++ != '&');
            break;
        }
    } while(*p && (ch = *p++) != '&');

    if (lp == *textp)
        return 0;

    *lp++ = 0;
    *textp = lp;
    return p;
}

static const char* markup_tags[] = {
    "b", "i", "p", "sub", "sup", "q", "s", "u",
};

static const char markup_ch[] = "*/`#^~-_";

const char* text_decode_markup(const char *p, char **textp, int maxlength)
{
    char ch, prev_ch;
    int h1, h2;
    uint32_t i;
    char *tp, *lp, *s;
    bool prev_lb, prev_ws, url, open_url, open_tag[8];

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
    prev_ch = 0;
    url = 0;
    open_url = 0;
    memset(open_tag, 0, sizeof(open_tag));

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

        if (ch == '<' || ch == '>') {
            *tp++ = '&';
            *tp++ = (ch == '<') ? 'l' : 'g';
            *tp++ = 't';
            ch = ';';
        }

        if (ch == '"') {
            *tp++ = '&';
            *tp++ = 'q';
            *tp++ = 'u';
            *tp++ = 'o';
            *tp++ = 't';
            ch = ';';
        }

        if (url) {
            if (ch == '\n' || iswhitespace(ch)) {
                tp += sprintf(tp, "\">");
                url = 0;
                open_url = 1;
                lp = tp + 1;
                continue;
            }
        }

        if (ch == '\n') {
            if (prev_lb)
                continue;

            prev_lb = 1;
            *tp++ = '<';
            *tp++ = 'b';
            *tp++ = 'r';
            ch = '>';
        } else if (iswhitespace(ch)) {
            if (prev_ws)
                continue;

            prev_ws = 1;
        } else if (isignore(ch)) {
            continue;
        } else {
            prev_lb = 0;
            prev_ws = 0;
            if (ch == prev_ch && !url) {
                s = strchr(markup_ch, ch);
                if (s) {
                    i = s - markup_ch;
                    if (i == 2 || !open_tag[2]) {
                        tp--;
                        if (open_tag[i])
                            tp += sprintf(tp, "</%s", markup_tags[i]);
                        else
                            tp += sprintf(tp, "<%s", markup_tags[i]);
                        open_tag[i] = !open_tag[i];
                        ch = '>';
                    }
                } else if (ch == '@') {
                    tp--;
                    if (!open_url)
                        tp += sprintf(tp, "<a href="), url = 1, ch = '"';
                    else
                        tp += sprintf(tp, "</a"), ch = '>';
                    open_url = 0;
                }
            }
            lp = tp + 1;
        }

        *tp++ = ch;
        prev_ch = ch;

        if (!--maxlength) {
            while (*p && *p++ != '&');
            break;
        }
    } while(*p && (ch = *p++) != '&');

    if (lp == *textp)
        return 0;

    if (url)
        lp += sprintf(lp, "\"></a>");
    else if (open_url)
        lp += sprintf(lp, "</a>");

    for (i = 0; i < sizeof(open_tag); i++)
        if (open_tag[i])
            lp += sprintf(lp, "</%s>", markup_tags[i]);

    *lp++ = 0;
    *textp = lp;
    return p;
}

const char* text_decode_name(const char *p, char *res, int res_len)
{
    char ch;

    if(*p != '=' || !*p)
        return 0;

    p++;

    if(*p == '&' || !*p)
        return 0;

    ch = *p++;
    do {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
              ch == '_'))
            return 0;
        if (!--res_len)
            return 0;
        *res++ = ch;
    } while(*p && (ch = *p++) != '&');

    *res++ = 0;
    return p;
}

const char* read_id(const char *p, uint32_t *id, char end)
{
    int n;
    uint32_t i, shift;

    i = 0;
    shift = 0;
    n = 8;
    do {
        if (--n < 0 || !(*p >= 'a' && *p <= 'p'))
            return 0;

        i |= ((*p - 'a') << shift);
        shift += 4;
    } while (*++p != end);

    *id = i;
    return p + 1;
}

char* print_id(char *p, uint32_t id)
{
    do {
        *p++ = 'a' + (id & 15);
        id >>= 4;
    } while (id);
    return p;
}

char* print_time(char *p, uint32_t time, uint32_t edit_time)
{
    time = current_time - time;

    if (time < 60) {
        p += sprintf(p, "%u seconds ago", time);
    } else {
        time /= 60;
        if (time < 60) {
            if (time == 1)
                p += sprintf(p, "a minute ago");
            else
                p += sprintf(p, "%u minutes ago", time);
        } else {
            time /= 60;
            if (time < 24) {
                if (time == 1)
                    p += sprintf(p, "an hour ago");
                else
                    p += sprintf(p, "%u hours ago", time);
            } else {
                time /= 24;
                if (time < 365) {
                    if (time == 1)
                        p += sprintf(p, "a day ago");
                    else
                        p += sprintf(p, "%u days ago", time);
                } else {
                    time /= 365;
                    if (time == 1)
                        p += sprintf(p, "a year ago");
                    else
                        p += sprintf(p, "%u years ago", time);
                }
            }
        }
    }

    if (edit_time)
        *p++ = '*';
    return p;
}
