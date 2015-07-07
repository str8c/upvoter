#include <stdint.h>
#include <stdbool.h>

enum {
    _sub,
    _frontpage,
    _domain,
};

enum {
    hot,
    rising,
    top,
    new,
};

typedef struct {
    uint32_t next, pass;
    char lower[12], name[13];
    char cookie[30];
    uint8_t lower_len;
    int karma_post, karma_comment;
    uint32_t new, new_comment, new_vote[2];
    uint32_t ninbox, new_inbox;
    uint8_t admin;
    uint8_t unused[924 + 1024 + 3];

    uint32_t vote_table[2][256];
} user_t;

typedef struct {
    uint32_t next;
    char name[28];
    uint32_t post[4], unused[4];
} sub_t;

typedef struct {
    uint32_t next;
    char name[28];
    uint32_t post[4], unused[4];
} domain_t;

typedef struct {
    uint32_t next[3][4], prev[3][3], nextu;
    uint32_t up, down, title, text, owner, time, sub, domain, ncomment;
    uint8_t has_protocol, unused[3];

    uint32_t comment[4], unused2[28];
} post_t;

typedef struct {
    uint32_t next[4], prev[3], nextu, nexti;
    uint32_t child[4], parent;
    uint32_t up, down, text, owner, time, post, unused[12];
} comment_t;

typedef struct {
    uint32_t next, id, nextu;
    uint8_t value, unused[3];
} vote_t;

user_t user[1024 * 256];
vote_t vote[1024 * 1024 * 4];
sub_t sub[1024 * 8];
domain_t domain[1024 * 8];

post_t post[1024 * 1024], *postp;
comment_t comment[1024 * 1024], *commentp;

char text[16 * 1024 * 1024], *textp;

struct {
    uint32_t post[4];
} frontpage;

user_t* get_user(const char *name, uint32_t pass, uint32_t ip, uint8_t *res);
user_t* get_user_name(const char **p);
user_t* get_user_cookie(const char *cookie);

int user_vote(user_t *u, bool type, uint32_t ip, uint32_t id, uint8_t value); //
uint8_t user_votestatus(user_t *u, bool type, uint32_t id);

int user_cvote(user_t *u, uint32_t ip, uint32_t id, uint8_t value); //
uint8_t user_cvotestatus(user_t *u, uint32_t id);

bool ip_postlimit(uint32_t ip, int karma);
bool ip_commentlimit(uint32_t ip, int karma);

sub_t* get_sub(const char *name, int name_len);

domain_t* get_domain(const char *str, uint8_t *res);
domain_t* get_domain_name(const char **p);

void time_event(void);
void init(void);
void save(void);

