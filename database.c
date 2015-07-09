#include "database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "main.h"
#include "util.h"

#define ip(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))

typedef struct {
    uint16_t ip;
    uint8_t next;
    uint8_t value[5];
} ipentry_t;

typedef struct {
    ipentry_t* entry[65536];
} iptable_t;

static uint32_t user_table[38 * 38 * 38 * 38];
static uint32_t sub_table[38 * 38 * 38 * 38];
static uint32_t domain_table[38 * 38 * 38 * 38];
static uint32_t nuser, nsub, ndomain, nvote, npost;

static iptable_t iptable;

static uint32_t last_clear;

static ipentry_t* ip_find(iptable_t *table, uint32_t ip)
{
    uint16_t low, high, i;
    ipentry_t *entry, *root;

    low = ip & 0xFFFF;
    high = ip >> 16;
    entry = root = table->entry[low];
    if (entry) {
        do {
            if(entry->ip == high)
                return entry;
        } while ((entry++)->next);

        i = entry - root;
        entry = realloc(root, (i + 1) * sizeof(*entry));
        if (!entry)
            return 0;

        table->entry[low] = entry;
        entry += i;

        (entry - 1)->next = 1;
    } else {
        entry = table->entry[low] = malloc(sizeof(*entry));
        if (!entry)
            return 0;
    }

    entry->ip = high;
    entry->next = 0;
    memset(entry->value, 0, sizeof(entry->value));
    return entry;
}

static void ip_clear(iptable_t *table)
{
    ipentry_t **p;
    int n;

    n = 65536;
    p = table->entry;
    do {
        free(*p); *p = 0;
    } while (p++, --n);
}

static uint32_t domain_name(char *p, uint32_t len, const char *str, uint8_t *res)
{
    char *s;
    char ch;
    bool dot;

    if (!memcmp(str, "https://", 8))
        str += 8, *res = 1;
    else if (!memcmp(str, "http://", 7))
        str += 7, *res = 1;
    else
        *res = 0;

    s = p;
    dot = 0;
    do {
        ch = *str++;
        if (!ch || ch == '/')
            break;
        if (ch == '.')
            dot = 1;
        else if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        else if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')))
            return 0;
    } while (--len && (*p++ = ch, 1));

    if (!dot)
        return 0;

    *p = 0;
    return (p - s);
}

static const char* valid_domain(uint32_t len, const char *str)
{
    char ch;

    do {
        ch = *str++;
        if (ch == '/')
            return str;

        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.'))
            return 0;
    } while (--len && ch);

    return 0;
}

static void generate_cookie(user_t *u)
{
    char *p;
    int len;

    len = u->lower_len;

    memcpy(u->cookie, u->lower, len);
    p = u->cookie + len;
    *p++ = '+';
    len = 16;
    do {
        *p++ = 'a' + rand() % 26;
    } while (--len);
}

user_t* get_user(const char *name, uint32_t pass, uint32_t ip, uint8_t *res)
{
    char lower[12];
    uint32_t hash, id;
    int len;
    user_t *u;
    ipentry_t *entry;

    entry = ip_find(&iptable, ip);
    if (!entry) {
        *res = 3;
        return 0;
    }

    len = makelower(lower, sizeof(lower), name);
    hash = hash_user(lower);
    id = user_table[hash];

    if (id != ~0u) {
        do {
            u = &user[id];
            if (!memcmp(u->lower, lower, sizeof(lower))) {
                if (entry->value[1] == 10) {
                    *res = 1;
                    return 0;
                }

                if (strcmp(text + u->pass, text + pass)) {
                    *res = 0;
                    entry->value[1]++;
                    return 0;
                }
                generate_cookie(u);
                *res = 0;
                return u;
            }

            id = u->next;
        } while (id != ~0u);

        if (entry->value[0] == 3) {
            *res = 2;
            return 0;
        }

        id = nuser++;
        u->next = id;
    } else {
        if (entry->value[0] == 3) {
            *res = 2;
            return 0;
        }

        id = nuser++;
        user_table[hash] = id;
    }

    entry->value[0]++;

    u = &user[id];
    u->next = ~0u;
    u->pass = pass;
    memcpy(u->lower, lower, 12);
    memcpy(u->name, name, 12);
    u->lower_len = len;

    u->new = u->new_comment = u->new_vote[0] = u->new_vote[1] = u->new_reply = u->new_priv = ~0u;
    memset(u->vote_table, 0xFF, sizeof(u->vote_table));

    generate_cookie(u);

    *res = 1;
    return u;
}

user_t* get_user_name(const char **p)
{
    char lower[12];
    uint32_t hash, id;
    user_t *u;

    *p = readlower(lower, sizeof(lower), *p);
    if (!*p)
        return 0;

    hash = hash_user(lower);
    id = user_table[hash];

    if (id != ~0u) {
        do {
            u = &user[id];
            if (!memcmp(u->lower, lower, sizeof(lower)))
                return u;

            id = u->next;
        } while (id != ~0u);
    }

    return 0;
}

user_t* get_user_cookie(const char *cookie)
{
    char lower[12], *p;
    uint32_t hash, id;
    user_t *u;

    if (!cookie)
        return 0;

    memset(lower, 0, sizeof(lower));
    p = lower;
    while (*cookie != '+') {
        if (!*cookie)
            return 0;
        *p++ = *cookie++;
    }
    cookie++;

    hash = hash_user(lower);
    id = user_table[hash];

    if (id == ~0u)
        return 0;

    do {
        u = &user[id];
        if (!memcmp(u->lower, lower, sizeof(lower))) {
            if (memcmp(u->cookie + u->lower_len + 1, cookie, 16))
                return 0;
            return u;
        }

        id = u->next;
    } while (id != ~0u);

    return 0;
}

int user_vote(user_t *u, bool type, uint32_t ip, uint32_t id, uint8_t value)
{
    uint32_t i;
    uint8_t prev;
    vote_t *v;
    ipentry_t *entry;

    entry = ip_find(&iptable, ip);
    if (!entry)
        return -1;

    i = u->vote_table[type][id & 255];

    if (i != ~0u) {
        do {
            v = &vote[i];
            if (v->id == id) {
                prev = v->value;
                v->value = value;
                return prev;
            }

            i = v->next;
        } while (i != ~0u);

        i = nvote++;
        v->next = i;
    } else {
        i = nvote++;
        u->vote_table[type][id & 255] = i;
    }

    v = &vote[i];
    v->next = ~0u;
    v->id = id;
    v->nextu = u->new_vote[type];
    u->new_vote[type] = i;
    v->value = value;

    return 0;

}

uint8_t user_votestatus(user_t *u, bool type, uint32_t id)
{
    uint32_t i;
    vote_t *v;

    i = u->vote_table[type][id & 255];
    if (i == ~0u)
        return 0;

    do {
        v = &vote[i];
        if (v->id == id)
            return v->value;

        i = v->next;
    } while (i != ~0u);
    return 0;
}

bool ip_postlimit(uint32_t ip, int karma)
{
    ipentry_t *entry;

    entry = ip_find(&iptable, ip);
    if (!entry)
        return 1;

    if (entry->value[2] >= 2 + karma / 128)
        return 1;

    if (entry->value[2] != 255)
        entry->value[2]++;
    return 0;
}

bool ip_commentlimit(uint32_t ip, int karma)
{
    ipentry_t *entry;

    entry = ip_find(&iptable, ip);
    if (!entry)
        return 1;

    if (entry->value[3] >= 4 + karma / 64)
        return 1;

    if (entry->value[3] != 255)
        entry->value[3]++;
    return 0;
}

bool ip_pmlimit(uint32_t ip)
{
    ipentry_t *entry;

    entry = ip_find(&iptable, ip);
    if (!entry)
        return 1;

    if (entry->value[4] >= 5)
        return 1;

    entry->value[4]++;
    return 0;
}

sub_t* get_sub_name(const char **p)
{
    const char *name;
    uint32_t hash, id;
    sub_t *s;

    name = *p;
    hash = hash_sub(p, sizeof(s->name));
    if (!hash)
        return 0;

    id = sub_table[hash];

    if (id != ~0u) {
        do {
            s = &sub[id];
            if (strcmp_slash(s->name, name))
                return s;

            id = s->next;
        } while (id != ~0u);

        id = nsub++;
        s->next = id;
    } else {
        id = nsub++;
        sub_table[hash] = id;
    }

    s = &sub[id];
    s->next = ~0u;
    memcpy(s->name, name, *p - name - 1);
    memset(s->post, 0xFF, sizeof(s->post));
    return s;
}

/* sub_t* get_sub(uint32_t id)
{
    return &sub[id];
} */

uint32_t sub_id(sub_t *s)
{
    return (s - sub);
}

domain_t* get_domain(const char *str, uint8_t *res)
{
    char name[28];
    int name_len;
    uint32_t hash, id;
    domain_t *d;

    name_len = domain_name(name, sizeof(name), str, res);
    if (!name_len)
        return 0;

    hash = hash_domain(name);
    id = domain_table[hash];

    if (id != ~0u) {
        do {
            d = &domain[id];
            if (!strcmp(d->name, name))
                return d;

            id = d->next;
        } while (id != ~0u);

        id = ndomain++;
        d->next = id;
    } else {
        id = ndomain++;
        domain_table[hash] = id;
    }

    d = &domain[id];
    d->next = ~0u;
    strcpy(d->name, name);
    memset(d->post, 0xFF, sizeof(d->post));

    return d;
}

domain_t* get_domain_name(const char **p)
{
    const char *name;
    uint32_t hash, id;
    domain_t *d;

    name = *p;
    if (!(*p = valid_domain(sizeof(d->name), *p)))
        return 0;

    hash = hash_domain_slash(name);
    id = domain_table[hash];

    if (id != ~0u) {
        do {
            d = &domain[id];
            if (strcmp_slash(d->name, name))
                return d;

            id = d->next;
        } while (id != ~0u);
    }

    return 0;
}

post_t* new_post(void)
{
    return &post[npost++];
}

post_t* get_post(uint32_t id)
{
    if (id >= npost)
        return 0;

    return &post[id];
}

void time_event(void)
{
    current_time = time(0);
    if (current_time - last_clear >= 300u) {
        last_clear = current_time;
        ip_clear(&iptable);
        save();
    }
}

void readfile(const char *path, void *data, uint32_t size, uint32_t *r_len)
{
    FILE *file;
    uint32_t len;

    *r_len = 0;

    file = fopen(path, "rb");
    if (!file)
        return;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (len <= size && fread(data, 1, len, file) == len)
        *r_len = len;
    return;
}

void writefile(const char *path, void *data, uint32_t size)
{
    FILE *file;

    file = fopen(path, "wb");
    if (!file)
        return;

    fwrite(data, 1, size, file);
    fclose(file);
}

#define loadfile(p, count) readfile(#p, p, sizeof(p), &len); count = len / sizeof(*p)
#define loadfile2(p) readfile(#p, p, sizeof(p), &len); assert(!len || len == sizeof(p)); \
    if (!len) memset(p, 0xFF, sizeof(p));
#define loadfile3(p) readfile(#p, &p, sizeof(p), &len); assert(!len || len == sizeof(p)); \
    if (!len) memset(&p, 0xFF, sizeof(p));
#define savefile(p, count) writefile(#p, p, (count) * sizeof(*p))
#define savefile2(p) writefile(#p, p, sizeof(p))
#define savefile3(p) writefile(#p, &p, sizeof(p))

void init(void)
{
    printf("sizes %lu %lu %lu %lu %lu %lu %lu\n", sizeof(user_t), sizeof(vote_t), sizeof(sub_t),
           sizeof(domain_t), sizeof(post_t), sizeof(comment_t), sizeof(privmsg_t));

    uint32_t len, n;

    current_time = time(0);
    last_clear = current_time;

    loadfile(user, nuser);
    loadfile(vote, nvote);
    loadfile(sub, nsub);
    loadfile(domain, ndomain);
    loadfile(post, npost);
    loadfile(comment, n); commentp = comment + n;
    loadfile(privmsg, n); privmsgp = privmsg + n;
    loadfile(text, n); textp = text + n;

    loadfile2(user_table);
    loadfile2(sub_table);
    loadfile2(domain_table);
    loadfile3(frontpage);
}

void save(void)
{
    savefile(user, nuser);
    savefile(vote, nvote);
    savefile(sub, nsub);
    savefile(domain, ndomain);
    savefile(post, npost);
    savefile(comment, commentp - comment);
    savefile(privmsg, privmsgp - privmsg);
    savefile(text, textp - text);

    savefile2(user_table);
    savefile2(sub_table);
    savefile2(domain_table);
    savefile3(frontpage);
}


