/*
this file needs a lot of cleaning up
*/

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "html.h"
#include "database.h"
#include "util.h"

#define strcopy(p, str) memcpy(p, str, sizeof(str) - 1), p += sizeof(str) - 1

enum {
    utab_submitted,
    utab_comments,
    utab_upvoted,
    utab_downvoted,
    utab_liked,
    utab_disliked,
    utab_msg,
    utab_replies,
    utab_inbox,
    utab_secret,
};

static const char* user_tabs[] = {
    "", "comments", "upvoted", "downvoted", "liked", "disliked", "msg", "replies", "inbox", SECRET, 0
};

static const char* domain_tabs[] = {
    "", "rising", "top", "new", 0
};

static const char* post_tabs[] = {
    "", "rising", "top", "new", SECRET, 0
};

static const char* sub_tabs[] = {
    "", "rising", "top", "new", "submit", 0
};

static char* topbar_end(char *p, user_t *user)
{
    strcopy(p, "<span style=\"float:right;margin-right:10px\">"
            "<a class=\"b1\" href=\"/\">frontpage</a> | ");
    if (user) {
    p += sprintf(p, "logged in as <a class=\"b1\" href=\"/u/%s/\">%s</a> | ", user->name, user->name);
    if (user->nreply + user->npriv)
        p += sprintf(p, "<a class=\"b2\" href=\"/u/%s/%s\">inbox (%u)</a>",
                     user->name, user->npriv ? "inbox" : "replies", user->nreply + user->npriv);
    else
        p += sprintf(p, "<a class=\"b1\" href=\"/u/%s/inbox\">inbox</a>", user->name);

    strcopy(p, " | <a class=\"b1\" href=\"/logout\">logout</a>");
    } else {
        strcopy(p, "<a class=\"b1\" href=\"/login\">login</a>");
    }
    strcopy(p, "</span></div>");

    return p;
}

static char* post_html(user_t *u, post_t *post, char *p, uint32_t id, uint32_t i, uint8_t type)
{
    char id_str[9], time_str[16], *pp;
    user_t *owner;
    const char *name, *dom;
    int karma;
    uint8_t vote;

    pp = print_id(id_str, id); *pp = 0;
    pp = print_time(time_str, post->time, post->edit_time); *pp = 0;

    owner = &user[post->owner];
    name = owner->name;
    karma = post->up - post->down;


    strcopy(p, "<div class=\"u\">");

    const char *ck = " checked";

    if (u) {
        vote = user_votestatus(u, 0, id);
        if (vote == 1)
            karma--;
        if (vote == 2)
            karma++;

        p += sprintf(p,
            "<div><form style=\"margin:0px\" method=\"POST\" action=\"/v/%s/\" target=\"h\" autocomplete=\"off\"> "
"<input class=\"l\" type=\"radio\" name=\"a\" value=\"0\" id=\"a%u\" onclick=\"this.form.submit()\"%s>"
"<input class=\"m\" type=\"radio\" name=\"a\" value=\"0\" id=\"b%u\" onclick=\"this.form.submit()\">"
"<input class=\"n\" type=\"radio\" name=\"a\" value=\"1\" id=\"c%u\" onclick=\"this.form.submit()\"%s>"
"<input class=\"o\" type=\"radio\" name=\"a\" value=\"2\" id=\"d%u\" onclick=\"this.form.submit()\"%s>"

            "<label class=\"p\" for=\"c%u\"></label><label class=\"r\" for=\"a%u\"></label><br>"
            "<span class=\"t\">%i</span><span class=\"r1\">%i</span><span class=\"s1\">%i</span><br>"
            "<label class=\"q\" for=\"d%u\"></label><label class=\"s\" for=\"b%u\"></label><br>"
            "</form></div>",
            id_str, i, vote == 0 ? ck : "", i, i, vote == 1 ? ck : "", i, vote == 2 ? ck : "",
            i, i, karma, karma + 1, karma - 1, i, i
        );
    } else {
        p += sprintf(p,
            "<div><input class=\"l\" type=\"radio\" checked>"
            "<span class=\"p\"></span><br>"
            "<span class=\"t\">%i</span><br>"
            "<span class=\"q\"></span><br></div>",
            karma);
    }

    if (post->domain != ~0u) {

            dom = domain[post->domain].name;

    p += sprintf(p,
        "<div>"
        "<a href=\"%s%s\">%s</a> "
        "<span class=\"c\">(<a class=\"b\" href=\"/d/%s/\">%s</a>)</span><br>",

        post->has_protocol ? "" : "http://",
        text + post->text, text + post->title, dom, dom
    );
    } else {
    p += sprintf(p,
        "<div>"
        "<a href=\"/%s/%s/\">%s</a> "
        "<span class=\"c\">(<a class=\"b\" href=\"/%s/\">self.%s</a>)</span><br>"
        "<input class=\"x\" type=\"checkbox\" id=\"s%u\" autocomplete=\"off\"%s>"
        "<label class=\"z\" for=\"s%u\">[+]</label>"
        "<label class=\"w\" for=\"s%u\">[-]</label> ",
        sub[post->sub].name, id_str, text + post->title, sub[post->sub].name, sub[post->sub].name,
        i, type == 2 ? "" : ck, i, i
    );
    }

    if (!(type & 1)) {
        p += sprintf(p,
        "<span class=\"d\">submitted %s by <a class=\"a\" href=\"/u/%s/\">%s</a></span><br>"
        "<a class=\"e\" href=\"/%s/%s/\">%u comments</a>",
        time_str, name, name, sub[post->sub].name, id_str, post->ncomment
        );
    } else {
        p += sprintf(p,
        "<span class=\"d\">submitted %s by <a class=\"a\" href=\"/u/%s/\">%s</a>"
        " to <a class=\"a\" href=\"/%s/\">/%s</a>"
        "</span><br>"
        "<a class=\"e\" href=\"/%s/%s/\">%u comments</a>",
        time_str, name, name, sub[post->sub].name, sub[post->sub].name,
        sub[post->sub].name, id_str, post->ncomment
        );
    }

    if (post->domain == ~0u) {
        p += sprintf(p, "<div class=\"w\">%s</div>", text + post->text);
    }

    strcopy(p, "</div></div>");

    return p;
}

//children count
static char* comm_html(user_t *u, char *p, uint32_t id, uint32_t *index, int sort, uint32_t depth)
{
    char id_str[9], time_str[16], *pp;
    comment_t *comm;
    user_t *owner;
    const char *name;
    uint32_t i;
    uint8_t vote;
    int karma;

    const char *ck = " checked";

    while (id != ~0u) {
        comm = &comment[id];
        i = *index; *index = i + 1;
        owner = &user[comm->owner];
        name = owner->name;

        pp = print_id(id_str, id); *pp = 0;
        pp = print_time(time_str, comm->time, comm->edit_time); *pp = 0;

        karma = comm->up - comm->down;
        if (u) {

        vote = user_votestatus(u, 1, id);
        if (vote == 1)
            karma--;
        if (vote == 2)
            karma++;

        p += sprintf(p,
            "<div%s>"
"<input class=\"x\" type=\"checkbox\" id=\"s%u\" autocomplete=\"off\">"
"<label class=\"z\" for=\"s%u\">[+]</label>"
"<label class=\"w\" for=\"s%u\">[-]</label> "
"<form style=\"margin:0px\" method=\"POST\" action=\"/v/%s/\" target=\"h\" autocomplete=\"off\">"
"<input class=\"l\" type=\"radio\" name=\"b\" value=\"0\" id=\"u%u\" onclick=\"this.form.submit()\"%s>"
"<input class=\"m\" type=\"radio\" name=\"b\" value=\"0\" id=\"v%u\" onclick=\"this.form.submit()\">"
"<input class=\"n\" type=\"radio\" name=\"b\" value=\"1\" id=\"w%u\" onclick=\"this.form.submit()\"%s>"
"<input class=\"o\" type=\"radio\" name=\"b\" value=\"2\" id=\"x%u\" onclick=\"this.form.submit()\"%s>"
"<a class=\"h\" %shref=\"/u/%s/\">%s</a> <span class=\"t\">%i</span><span class=\"r1\">%i</span><span class=\"s1\">%i</span> points %s "
"(<label class=\"p\" for=\"w%u\">good</label><label class=\"r\" for=\"u%u\">good</label>, <label class=\"q\" for=\"x%u\">bad</label><label class=\"s\" for=\"v%u\">bad</label>)<br>"
"</form>"
"<div class=\"w\">"
"<span class=\"k\">%s</span><br>",

            /*"<div%s>"
            "<input class=\"x\" type=\"checkbox\" id=\"s%u\" hidden>"
            "<div class=\"z\">"
            "<label for=\"s%u\">[+]</label> "
            "<span class=\"j\"><a class=\"i\" href=\"/u/%s\">%s</a> %i points %s</span>"
            "</div>"
            "<div class=\"w\">"
            "<label for=\"s%u\">[-]</label> "
            "<a class=\"h\" href=\"/u/%s\">%s</a> %i points %s<br>"
            "<span class=\"k\">%s</span><br>", */
            depth ? " class=\"y\"" : "", i, i, i, id_str,
            i, vote == 0 ? ck : "", i, i, vote == 1 ? ck : "", i, vote == 2 ? ck : "",
            user[comm->owner].admin ? " style=\"color:#D00\"" : "",
            name, name, karma, karma + 1, karma - 1, time_str, i, i, i, i, text + comm->text
            );
        } else {
            p += sprintf(p,
            "<div%s>"
"<input class=\"x\" type=\"checkbox\" id=\"s%u\">"
"<label class=\"z\" for=\"s%u\">[+]</label>"
"<label class=\"w\" for=\"s%u\">[-]</label> "
"<input class=\"l\" type=\"radio\" checked>"
"<a class=\"h\" %shref=\"/u/%s/\">%s</a> <span class=\"t\">%i</span> points %s "
"<div class=\"w\">"
"<span class=\"k\">%s</span><br>",
                depth ? " class=\"y\"" : "", i, i, i,
                user[comm->owner].admin ? " style=\"color:#D00\"" : "",
                name, name, karma, time_str, text + comm->text
            );
        }


        if (u) {
            p += sprintf(p,
            "<input class=\"x2\" type=\"radio\" name=\"x%u\" id=\"r%u\" checked>"
            "<input class=\"x1\" type=\"radio\" name=\"x%u\" id=\"t%u\">"
            "<label class=\"y1\" for=\"t%u\">%s</label> ", i, i, i, i, i,
            u == owner ? "edit" : "reply");

            p += sprintf(p,
            "<div class=\"y2\">"
            "<form method=\"POST\">"
            "<input type=\"hidden\" name=\"b\" value=\"%s\">"
            "<textarea name=\"a\" class=\"text\" maxlength=\"2048\"></textarea><br>"
            "<input type=\"submit\" value=\"Submit\">"
            "</form>"
            " <label for=\"r%u\">cancel</label></div>",
            id_str, i);
        } else {
            strcopy(p, "<br>");
        }

        p = comm_html(u, p, comm->child[sort], index, sort, depth + 1);
        strcopy(p, "</div></div>");

        id = comm->next[sort];
    }

    return p;
}

static char* comm_html2(user_t *u, comment_t *comm, char *p, uint32_t id, uint32_t i)
{
    char id_str[9], id_str_post[9], time_str[16], *pp;
    const char *name, *sname;
    uint8_t vote;
    int karma;
    post_t *pt;

    const char *ck = " checked";

    name = user[comm->owner].name;

    pp = print_id(id_str, id); *pp = 0;
    pp = print_id(id_str_post, comm->post); *pp = 0;
    pp = print_time(time_str, comm->time, comm->edit_time); *pp = 0;

    pt = &post[comm->post];
    sname = sub[pt->sub].name;

    karma = comm->up - comm->down;
    if (u) {

    vote = user_votestatus(u, 1, id);
    if (vote == 1)
        karma--;
    if (vote == 2)
        karma++;

    p += sprintf(p,
        "<div>"
"<input class=\"x\" type=\"checkbox\" id=\"s%u\" autocomfaplete=\"off\">"
"<label class=\"z\" for=\"s%u\">[+]</label>"
"<label class=\"w\" for=\"s%u\">[-]</label> "
"<form style=\"margin:0px\" method=\"POST\" action=\"/v/%s/\" target=\"h\" autocomplete=\"off\">"
"<input class=\"l\" type=\"radio\" name=\"b\" value=\"0\" id=\"u%u\" onclick=\"this.form.submit()\"%s>"
"<input class=\"m\" type=\"radio\" name=\"b\" value=\"0\" id=\"v%u\" onclick=\"this.form.submit()\">"
"<input class=\"n\" type=\"radio\" name=\"b\" value=\"1\" id=\"w%u\" onclick=\"this.form.submit()\"%s>"
"<input class=\"o\" type=\"radio\" name=\"b\" value=\"2\" id=\"x%u\" onclick=\"this.form.submit()\"%s>"
"<a class=\"h\" href=\"/u/%s/\">%s</a> <span class=\"t\">%i</span><span class=\"r1\">%i</span><span class=\"s1\">%i</span> points %s "
"(<label class=\"p\" for=\"w%u\">good</label><label class=\"r\" for=\"u%u\">good</label>, <label class=\"q\" for=\"x%u\">bad</label><label class=\"s\" for=\"v%u\">bad</label>) "
"</form>"
"in <a class=\"a\" href=\"/%s/%s/\">%s</a> to <a class=\"a\" href=\"/%s/\">/%s</a><br>"
"<div class=\"w\">"
"<span class=\"k\">%s</span><br>",

        /*"<div%s>"
        "<input class=\"x\" type=\"checkbox\" id=\"s%u\" hidden>"
        "<div class=\"z\">"
        "<label for=\"s%u\">[+]</label> "
        "<span class=\"j\"><a class=\"i\" href=\"/u/%s\">%s</a> %i points %s</span>"
        "</div>"
        "<div class=\"w\">"
        "<label for=\"s%u\">[-]</label> "
        "<a class=\"h\" href=\"/u/%s\">%s</a> %i points %s<br>"
        "<span class=\"k\">%s</span><br>", */
        i, i, i, id_str,
        i, vote == 0 ? ck : "", i, i, vote == 1 ? ck : "", i, vote == 2 ? ck : "",
        name, name, karma, karma + 1, karma - 1, time_str, i, i, i, i,
        sname, id_str_post, text + pt->title, sname, sname, text + comm->text
        );
    } else {
        p += sprintf(p,
        "<div>"
"<input class=\"x\" type=\"checkbox\" id=\"s%u\">"
"<label class=\"z\" for=\"s%u\">[+]</label>"
"<label class=\"w\" for=\"s%u\">[-]</label> "
"<input class=\"l\" type=\"radio\" checked>"
"<a class=\"h\" href=\"/u/%s/\">%s</a> <span class=\"t\">%i</span> points %s "
"in <a class=\"a\" href=\"/%s/%s/\">%s</a> to <a class=\"a\" href=\"/%s/\">/%s</a><br>"
"<div class=\"w\">"
"<span class=\"k\">%s</span><br>",
            i, i, i, name, name, karma, time_str,
            sname, id_str_post, text + pt->title, sname, sname, text + comm->text
        );
    }

    strcopy(p, "<br>");

    strcopy(p, "</div></div>");

    return p;
}

static char* priv_html(privmsg_t *msg, char *p, uint32_t i)
{
    (void) i;
    const char *name;
    char time_str[16], *pp;

    name = user[msg->from].name;

    pp = print_time(time_str, msg->time, 0); *pp = 0;

    p += sprintf(p,
                 "<div>from <a class=\"h\" href=\"/u/%s/\">%s</a>"
                 " %s<br><span class=\"k\">%s</span></div>",
                name, name, time_str, text + msg->text);

    return p;
}

static void insert(sub_t *s, post_t *pt, uint32_t id)
{
    uint32_t i, *pi;
    int k, w, z;
    post_t *p;
    double m;

    for (z = 0; z < 4; z++) {
        for (w = 0; w < 2 + (pt->domain != ~0u); w++) {
            if (w == 0)
                pi = &s->post[z];
            else if (w == 1)
                pi = &frontpage.post[z];
            else
                pi = &domain[pt->domain].post[z];

            i = *pi;
            if (i == ~0u) { /* first post */
                *pi = id;
                pt->next[w][z] = ~0u;
                pt->prev[w][z] = ~0u;
                continue;
            }

            do {
                p = &post[i];
                k = (int) p->up - p->down;

                if (z == 3)
                    break;

                if (z == 0)
                    m = exp2((double) ((int) pt->time -  (int) p->time) / 86400.0);
                else if (z == 1)
                    m = exp2((double) ((int) pt->time -  (int) p->time) / 3600.0);
                else
                    m = 1.0;

                if ((double) k <= m)
                    break;

                i = p->next[w][z];
            } while (i != ~0u);

            if (i == ~0u) {
                pt->next[w][z] = ~0u;
                pt->prev[w][z] = (p - post);
                p->next[w][z] = id;
                continue;
            }

            pt->next[w][z] = i;
            pt->prev[w][z] = p->prev[w][z];
            p->prev[w][z] = id;
            if (pt->prev[w][z] != ~0u)
                post[pt->prev[w][z]].next[w][z] = id;
            else
                *pi = id;
        }
    }
}

static void upvote(post_t *pt, uint32_t id)
{
    uint32_t i, id_prev, id_next;
    int j, k, w, z;
    post_t *p;
    double m;

    for (z = 0; z < 3; z++) {
        for (w = 0; w < 2 + (pt->domain != ~0u); w++) {
            i = pt->prev[w][z];
            if (i == ~0u)
                continue;

            j = (int) pt->up - pt->down;
            do {
                p = &post[i];
                k = (int) p->up - p->down;

                if (z == 0)
                    m = exp2((double) ((int) pt->time -  (int) p->time) / 86400.0);
                else if (z == 1)
                    m = exp2((double) ((int) pt->time -  (int) p->time) / 3600.0);
                else
                    m = 1.0;

                if ((double) k > (double) j * m)
                    break;

                i = p->prev[w][z];
            } while (i != ~0u);

            if (i == pt->prev[w][z]) /* no move */
                continue;

            /* detach */
            id_prev = pt->prev[w][z];
            id_next = pt->next[w][z];

            post[id_prev].next[w][z] = id_next;

            if (id_next != ~0u)
                post[id_next].prev[w][z] = id_prev;

            /* insert (below p) */
            if (i == ~0u) { /* first */
                if (w == 0)
                    id_next = sub[pt->sub].post[z];
                else if (w == 1)
                    id_next = frontpage.post[z];
                else
                    id_next = domain[pt->domain].post[z];

                pt->prev[w][z] = ~0u;
                pt->next[w][z] = id_next;

                post[id_next].prev[w][z] = id;

                if (w == 0)
                    sub[pt->sub].post[z] = id;
                else if (w == 1)
                    frontpage.post[z] = id;
                else
                    domain[pt->domain].post[z] = id;
                continue;
            }

            id_next = p->next[w][z];

            pt->prev[w][z] = i;
            pt->next[w][z] = id_next;

            post[id_next].prev[w][z] = id;
            p->next[w][z] = id;
        }
    }
}

static void downvote(post_t *pt, uint32_t id)
{
    uint32_t i, id_prev, id_next;
    int j, k, w, z;
    post_t *p;
    double m;

    for (z = 0; z < 3; z++) {
        for (w = 0; w < 2 + (pt->domain != ~0u); w++) {
            i = pt->next[w][z];
            if (i == ~0u)
                continue;

            j = (int) pt->up - pt->down;
            do {
                p = &post[i];
                k = (int) p->up - p->down;

                if (z == 0)
                    m = exp2((double) ((int) pt->time -  (int) p->time) / 86400.0);
                else if (z == 1)
                    m = exp2((double) ((int) pt->time -  (int) p->time) / 3600.0);
                else
                    m = 1.0;

                if ((double) k <= (double) j * m)
                    break;

                i = p->next[w][z];
            } while (i != ~0u);

            if (i == pt->next[w][z]) /* no move */
                continue;

            /* detach */
            id_prev = pt->prev[w][z];
            id_next = pt->next[w][z];

            if (id_prev != ~0u) {
                post[id_prev].next[w][z] = id_next;
            } else {
                if (w == 0)
                    sub[pt->sub].post[z] = id_next;
                else if (w == 1)
                    frontpage.post[z] = id_next;
                else
                    domain[pt->domain].post[z] = id_next;
            }

            post[id_next].prev[w][z] = id_prev;

            /* insert (above p) */
            if (i == ~0u) { /* bottom */
                id_prev = (p - post);
                p->next[w][z] = id;

                pt->next[w][z] = ~0u;
                pt->prev[w][z] = id_prev;
                continue;
            }

            id_prev = p->prev[w][z];

            pt->prev[w][z] = id_prev;
            pt->next[w][z] = i;

            post[id_prev].next[w][z] = id;
            p->prev[w][z] = id;
        }
    }
}

static bool submit(pageinfo_t *info, sub_t *s, user_t *owner, const char *p, uint8_t *res)
{
    post_t *pt;
    domain_t *dom;
    char *textp_old;
    uint32_t id;
    bool textpost;

    textp_old = textp;
    pt = new_post();
    id = pt - post;
    *res = 0;

    if (*p++ != 'a')
        goto fail;

    pt->title = textp - text;
    p = text_decode(p, &textp, 128, decode_title);
    if (!p)
        goto fail;

    if (*p != 'b' && *p != 'c')
        goto fail;

    textpost = (*p == 'c');
    p++;

    pt->text = textp - text;
    if (textpost)
        p = text_decode_markup(p, &textp, 2048);
    else
        p = text_decode(p, &textp, 256, decode_url);

    if (!p)
        goto fail;

    if (!textpost) {
        dom = get_domain(text + pt->text, &pt->has_protocol);
        if (!dom) {
            *res = 1;
            goto fail;
        }
    }

    if (ip_postlimit(info->ip, owner->karma_post)) {
        *res = 2;
        goto fail;
    }

    pt->owner = (owner - user);
    pt->time = current_time;
    pt->domain = textpost ? ~0u : (dom - domain);
    pt->sub = (s - sub);
    memset(pt->comment, 0xFF, sizeof(pt->comment));
    pt->up = 1;

    pt->nextu = owner->new;
    owner->new = id;

    user_vote(owner, 0, info->ip, id, 1);

    insert(s, pt, id);

    info->redirect = "new";
    info->redirect_len = 3;
    return 1;
fail:
    textp = textp_old;
    return 0;
}

static void cinsert(post_t *pt, comment_t *comm, uint32_t id)
{
    uint32_t i, *pi;
    int k, z;
    comment_t *c;
    double m;

    for (z = 0; z < 3; z++) {
        if (comm->parent == ~0u)
            pi = &pt->comment[z];
        else
            pi = &comment[comm->parent].child[z];

        i = *pi;
        if (i == ~0u) { /* first post */
            *pi = id;
            comm->next[z] = ~0u;
            comm->prev[z] = ~0u;
            continue;
        }

        do {
            c = &comment[i];
            k = (int) c->up - c->down;

            if (z == 0)
                m = exp2((double) ((int) comm->time -  (int) c->time) / 86400.0);
            else if (z == 1)
                m = exp2((double) ((int) comm->time -  (int) c->time) / 3600.0);
            else
                m = 1.0;

            if ((double) k <= m)
                break;

            i = c->next[z];
        } while (i != ~0u);

        if (i == ~0u) {
            comm->next[z] = ~0u;
            comm->prev[z] = (c - comment);
            c->next[z] = id;
            continue;
        }

        comm->next[z] = i;
        comm->prev[z] = c->prev[z];
        c->prev[z] = id;
        if (comm->prev[z] != ~0u)
            comment[comm->prev[z]].next[z] = id;
        else
            *pi = id;
    }
}

static void cupvote(comment_t *comm, uint32_t id)
{
    uint32_t i, id_prev, id_next;
    int j, k, z;
    comment_t *c;
    double m;

    for (z = 0; z < 3; z++) {
        i = comm->prev[z];
        if (i == ~0u)
            continue;

        j = (int) comm->up - comm->down;
        do {
            c = &comment[i];
            k = (int) c->up - c->down;

            if (z == 0)
                m = exp2((double) ((int) comm->time -  (int) c->time) / 86400.0);
            else if (z == 1)
                m = exp2((double) ((int) comm->time -  (int) c->time) / 3600.0);
            else
                m = 1.0;

            if ((double) k > (double) j * m)
                break;

            i = c->prev[z];
        } while (i != ~0u);

        if (i == comm->prev[z]) /* no move */
            continue;

        /* detach */
        id_prev = comm->prev[z];
        id_next = comm->next[z];

        comment[id_prev].next[z] = id_next;

        if (id_next != ~0u)
            comment[id_next].prev[z] = id_prev;

        /* insert (below p) */
        if (i == ~0u) { /* first */
            if (comm->parent == ~0u)
                id_next = post[comm->post].comment[z];
            else
                id_next = comment[comm->parent].child[z];

            comm->prev[z] = ~0u;
            comm->next[z] = id_next;

            comment[id_next].prev[z] = id;

            if (comm->parent == ~0u)
                post[comm->post].comment[z] = id;
            else
                comment[comm->parent].child[z] = id;

            continue;
        }

        id_next = c->next[z];

        comm->prev[z] = i;
        comm->next[z] = id_next;

        comment[id_next].prev[z] = id;
        c->next[z] = id;
    }
}

static void cdownvote(comment_t *comm, uint32_t id)
{
    uint32_t i, id_prev, id_next;
    int j, k, z;
    comment_t *c;
    double m;

    for (z = 0; z < 3; z++) {
        i = comm->next[z];
        if (i == ~0u)
            continue;

        j = (int) comm->up - comm->down;
        do {
            c = &comment[i];
            k = (int) c->up - c->down;

            if (z == 0)
                m = exp2((double) ((int) comm->time -  (int) c->time) / 86400.0);
            else if (z == 1)
                m = exp2((double) ((int) comm->time -  (int) c->time) / 3600.0);
            else
                m = 1.0;

            if ((double) k <= (double) j * m)
                break;

            i = c->next[z];
        } while (i != ~0u);

        if (i == comm->next[z]) /* no move */
            continue;

        /* detach */
        id_prev = comm->prev[z];
        id_next = comm->next[z];

        if (id_prev != ~0u) {
            comment[id_prev].next[z] = id_next;
        } else {
            if (comm->parent == ~0u)
                post[comm->post].comment[z] = id_next;
            else
                comment[comm->parent].child[z] = id_next;
        }

        comment[id_next].prev[z] = id_prev;

        /* insert (above p) */
        if (i == ~0u) { /* bottom */
            id_prev = (c - comment);
            c->next[z] = id;

            comm->next[z] = ~0u;
            comm->prev[z] = id_prev;
            continue;
        }

        id_prev = c->prev[z];

        comm->prev[z] = id_prev;
        comm->next[z] = i;

        comment[id_prev].next[z] = id;
        c->prev[z] = id;
    }
}

static bool submit_comment(pageinfo_t *info, post_t *pt, user_t *owner, const char *p, uint8_t *res)
{
    comment_t *comm, *parent;
    user_t *parent_u;
    char *textp_old;
    uint32_t id, parent_id, t;

    textp_old = textp;
    parent = 0;
    *res = 0;

    if (*p == 'b') {
        p++;
        if (*p++ != '=')
            goto fail;

        p = read_id(p, &parent_id, '&');
        if (!p)
            goto fail;

        parent = &comment[parent_id];
        if (parent_id >= (commentp - comment) || parent->post != pt - post)
            goto fail;
    }

    if (*p++ != 'a')
        goto fail;

    t = textp - text;
    p = text_decode_markup(p, &textp, 2048);
    if (!p)
        goto fail;

    if (ip_commentlimit(info->ip, owner->karma_comment)) {
        *res = 1;
        goto fail;
    }

    if (!parent) {
        parent_u = &user[pt->owner];
    } else {
        parent_u = &user[parent->owner];
        if (parent_u == owner) {
            comm = &comment[parent_id];
            comm->text = t;
            comm->edit_time = current_time;
            info->redirect_len = 0;
            return 1;
        }
    }

    comm = commentp;
    id = comm - comment;
    comm->text = t;

    pt->ncomment++;


    if (!parent) {
        comm->next[new] = pt->comment[new];
        comm->parent = ~0u;
        pt->comment[new] = id;
    } else {
        comm->next[new] = parent->child[new];
        comm->parent = parent_id;
        parent->child[new] = id;
    }

    comm->nexti = parent_u->new_reply;
    parent_u->new_reply = id;
    parent_u->nreply++;

    memset(comm->child, ~0u, sizeof(comm->child));

    comm->owner = (owner - user);
    comm->time = current_time;
    comm->post = (pt - post);
    comm->up = 1;

    comm->nextu = owner->new_comment;
    owner->new_comment = id;
    user_vote(owner, 1, info->ip, id, 1);

    cinsert(pt, comm, id);

    commentp++;

    info->redirect_len = 0;
    return 1;
fail:
    textp = textp_old;
    return 0;
}

static bool submit_priv(pageinfo_t *info, user_t *to, user_t *from, const char *p, uint8_t *res)
{
    privmsg_t *msg;
    char *textp_old;
    uint32_t id;

    textp_old = textp;
    msg = privmsgp;
    id = msg - privmsg;
    *res = 0;

    if (*p++ != 'a')
        goto fail;

    msg->text = textp - text;
    p = text_decode_markup(p, &textp, 2048);
    if (!p)
        goto fail;

    if (ip_pmlimit(info->ip)) {
        *res = 1;
        goto fail;
    }

    msg->from = (from - user);
    msg->time = current_time;

    msg->next = to->new_priv;
    to->new_priv = id;
    to->npriv++;

    privmsgp++;

    info->redirect_len = 0;
    return 1;
fail:
    textp = textp_old;
    return 0;
}

static int post_page(pageinfo_t *info, sub_t *sub, user_t *u, post_t *pt, const char *content, int sort)
{
    char *p;
    uint32_t index;
    uint8_t res;

    p = info->buf;

    strcopy(p, html_head);

    p += sprintf(p, "<title>/%s: %s</title>", sub->name, text + pt->title);

    strcopy(p, html_body);

    p += sprintf(p, "<a style=\"color:#000\" href=\"..\">/%s</a> |"
                 "<a class=\"g\" href=\"\">comments</a>|", sub->name);

    p = topbar_end(p, u);

    p = post_html(u, pt, p, (pt - post), 0, 2);

    strcopy(p, "<div style=\"margin:5px 0px 0px 10px;font-size:12px\">");

    if (content && u) {
        if (!submit_comment(info, pt, u, content, &res)) {
            if (res)
                strcopy(p, "<span class=\"b2\">reached comment limit, wait 5 minutes</span>");
            else
                strcopy(p, "<span class=\"b2\">invalid input</span>");
        }
    }

    if (u) {
        strcopy(p,
                "<form style=\"display:block\" method=\"POST\">"
                "<textarea name=\"a\" class=\"text\" maxlength=\"2048\"></textarea><br>"
                "<input type=\"submit\" value=\"Submit\">"
                "</form>"
        );
    }

    p += sprintf(p,
                 "<div style=\"margin-bottom:5px\">sort by: <a class=\"b%C\" href=\".\">hot</a>, "
                 "<a class=\"b%C\" href=\"new\">new</a>, <a class=\"b%C\" href=\"rising\">rising</a>, "
                 "<a class=\"b%C\" href=\"top\">top</a></div>",
                 '1' + (sort == hot), '1' + (sort == new), '1' + (sort == rising), '1' + (sort == top)
    );


    p = comm_html(u, p, pt->comment[sort], &index, sort, 0);

    strcopy(p, html_post_end);
    return p - info->buf;
}

static int sub_page(pageinfo_t *info, sub_t *sub, user_t *user, const char *content, int tab,
                    uint32_t page)
{
    char *p;
    uint32_t id, i;
    uint8_t res;
    post_t *pt;

    p = info->buf;

    strcopy(p, html_head);
    p += sprintf(p, "<title>/%s</title>", sub ? sub->name : "");
    strcopy(p, html_body);
    p += sprintf(p, "<a style=\"color:#000\" href=\".\">/%s</a> |", sub ? sub->name : "");
    p += sprintf(p,
        "<a class=\"%C\" href=\".\">hot</a>|"
        "<a class=\"%C\" href=\"new\">new</a>|"
        "<a class=\"%C\" href=\"rising\">rising</a>|"
        "<a class=\"%C\" href=\"top\">top</a>|"
        "<a class=\"%C\" href=\"submit\">submit</a>",
        'f' + (tab == 0), 'f' + (tab == 3), 'f' + (tab == 1), 'f' + (tab == 2), 'f' + (tab == 4)
    );

    p = topbar_end(p, user);

    if (tab == 4) {
        strcopy(p, "<div style=\"margin:5px 0px 0px 10px;font-size:12px\">");
        if (!user) {
            strcopy(p, "login to submit");
        } else if (!sub) {
            strcopy(p, "choose a sub: <a href=\"/default/submit\">/default</a>");
        } else {
            strcopy(p, html_submit);
            if (content && user) {
                if (!submit(info, sub, user, content, &res)) {
                    if (res == 0)
                        strcopy(p, " <span class=\"b2\">invalid input</span>");
                    else if (res == 1)
                        strcopy(p, " <span class=\"b2\">invalid url</span>");
                    else
                        strcopy(p, " <span class=\"b2\">reached post limit, wait 5 minutes</span>");
                }
            }
        }

        strcopy(p, "</div>");

        goto end;
    }

    id = sub ? sub->post[tab] : frontpage.post[tab];
    i = 0;
    while (id != ~0u && i < 25 * (page + 1)) {
        pt = &post[id];
        if (i >= 25 * page)
            p = post_html(user, pt, p, id, i, sub == 0);
        i++;
        id = pt->next[!sub][tab];
    }

end:
    strcopy(p, html_main_end);
    return p - info->buf;
}

static int domain_page(pageinfo_t *info, domain_t *d, user_t *user, int tab, uint32_t page)
{
    char *p;
    uint32_t id, i;
    post_t *pt;

    p = info->buf;

    strcopy(p, html_head);
    p += sprintf(p, "<title>/d/%s</title>", d->name);
    strcopy(p, html_body);
    p += sprintf(p, "<a style=\"color:#000\" href=\".\">/d/%s</a> |", d->name);
    p += sprintf(p,
        "<a class=\"%C\" href=\".\">hot</a>|"
        "<a class=\"%C\" href=\"new\">new</a>|"
        "<a class=\"%C\" href=\"rising\">rising</a>|"
        "<a class=\"%C\" href=\"top\">top</a>",
        'f' + (tab == 0), 'f' + (tab == 3), 'f' + (tab == 1), 'f' + (tab == 2)
    );

    p = topbar_end(p, user);

    id = d->post[tab];
    i = 0;
    while (id != ~0u && i < 25 * (page + 1)) {
        pt = &post[id];
        if (i >= 25 * page)
            p = post_html(user, pt, p, id, i, 1);

        i++;
        id = pt->next[2][tab];
    }

    strcopy(p, html_main_end);
    return p - info->buf;
}

static int user_page(pageinfo_t *info, user_t *u, user_t *login, const char *content, int tab,
                     uint32_t page)
{
    char *p;
    uint32_t id, i;
    uint8_t res;
    post_t *pt;
    comment_t *comm;
    vote_t *v;
    privmsg_t *pm;

    if (tab >= 7) {
        if (u != login)
            return -1;

        if (tab == 7)
            u->nreply = 0;
        else
            u->npriv = 0;
    }
    else if (tab == 6 && !login)
        return -1;

    p = info->buf;

    strcopy(p, html_head);
    p += sprintf(p, "<title>/u/%s</title>", u->name);
    strcopy(p, html_body);
    p += sprintf(p, "<a style=\"color:#000\" href=\".\">/u/%s</a> (%i, %i) ", u->name,
                 u->karma_post, u->karma_comment);
    p += sprintf(p,
        "|<a class=\"%C\" href=\".\">submitted</a>"
        "|<a class=\"%C\" href=\"comments\">comments</a>"
        "|<a class=\"%C\" href=\"upvoted\">upvoted</a>"
        "|<a class=\"%C\" href=\"downvoted\">downvoted</a>"
        "|<a class=\"%C\" href=\"liked\">liked</a>"
        "|<a class=\"%C\" href=\"disliked\">disliked</a>",
        'f' + (tab == 0), 'f' + (tab == 1), 'f' + (tab == 2), 'f' + (tab == 3), 'f' + (tab == 4),
                 'f' + (tab == 5)
    );

    if (login == u) {
    p += sprintf(p,
        "|<a class=\"%C\" href=\"replies\">replies (%u)</a>"
        "|<a class=\"%C\" href=\"inbox\">private (%u)</a>",
        'f' + (tab == 7), u->nreply, 'f' + (tab == 8), u->npriv
    );
    } else if (login) {
    p += sprintf(p,
        "|<a class=\"%C\" href=\"msg\">send message</a>",
        'f' + (tab == 6)
    );
    }

    p = topbar_end(p, login);

    i = 0;
    if (tab == 0) {
        id = u->new;
        while (id != ~0u && i < 25 * (page + 1)) {
            pt = &post[id];
            if (i >= 25 * page)
                p = post_html(login, pt, p, id, i, 1);
            i++;
            id = pt->nextu;
        }
    } if (tab == 1) {
        strcopy(p, "<div style=\"margin:5px 0px 0px 10px;font-size:12px\">");
        id = u->new_comment;
        while (id != ~0u && i < 25 * (page + 1)) {
            comm = &comment[id];
            if (i >= 25 * page)
                p = comm_html2(login, comm, p, id, i);
            i++;
            id = comm->nextu;
        }
        strcopy(p, "</div>");
    } else if (tab <= 3) {
        id = u->new_vote[0];
        while (id != ~0u && i < 25 * (page + 1)) {
            v = &vote[id];
            if (v->value == tab - 1) {
                pt = &post[v->id];
                if (i >= 25 * page)
                    p = post_html(login, pt, p, v->id, i, 1);
                i++;
            }
            id = v->nextu;
        }
    } else if (tab <= 5) {
        strcopy(p, "<div style=\"margin:5px 0px 0px 10px;font-size:12px\">");
        id = u->new_vote[1];
        while (id != ~0u && i < 25 * (page + 1)) {
            v = &vote[id];
            if (v->value == tab - 3) {
                comm = &comment[v->id];
                if (i >= 25 * page)
                    p = comm_html2(login, comm, p, v->id, i);
                i++;
            }
            id = v->nextu;
        }
        strcopy(p, "</div>");
    } else if (tab == 6) {
        strcopy(p, "<form method=\"POST\">"
                "<textarea name=\"a\" class=\"text\" maxlength=\"2048\"></textarea><br>"
                "<input type=\"submit\" value=\"Submit\">"
                "</form>");

        if (content) {
            if (!submit_priv(info, u, login, content, &res)) {
                if (res == 0)
                    strcopy(p, " <span class=\"b2\">invalid input</span>");
                else
                    strcopy(p, " <span class=\"b2\">reached limit, wait 5 minutes</span>");
            } else {
                strcopy(p, " <span class=\"b2\">message sent</span>");
            }
        }
    } else if (tab == 7) {
        strcopy(p, "<div style=\"margin:5px 0px 0px 10px;font-size:12px\">");
        id = u->new_reply;
        while (id != ~0u && i < 25 * (page + 1)) {
            comm = &comment[id];
            if (i >= 25 * page)
                p = comm_html2(login, comm, p, id, i);
            i++;
            id = comm->nexti;
        }
        strcopy(p, "</div>");
    } else {
        strcopy(p, "<div style=\"margin:5px 0px 0px 10px;font-size:12px\">");
        id = u->new_priv;
        while (id != ~0u && i < 25 * (page + 1)) {
            pm = &privmsg[id];
            if (i >= 25 * page)
                p = priv_html(pm, p, i);
            i++;
            id = pm->next;
        }
        strcopy(p, "</div>");
    }

    strcopy(p, html_main_end);
    return p - info->buf;
}

static int login_page(pageinfo_t *info, const char *post, user_t *user)
{
    char *p, *textp_old;
    char name[13];
    uint32_t pass;
    uint8_t res;
    user_t *u;

    p = info->buf;

    strcopy(p, html_login_head);

    if (!post)
        goto end;

    if (*post++ != 'a') {
        strcopy(p, "<div style=\"color:#D00\">Invalid input</div>");
        goto end;
    }

    post = text_decode_name(post, name, sizeof(name));
    if (!post) {
        strcopy(p, "<div style=\"color:#D00\">Invalid input</div>");
        goto end;
    }

    if (*post++ != 'b') {
        strcopy(p, "<div style=\"color:#D00\">Invalid input</div>");
        goto end;
    }

    textp_old = textp;
    pass = textp - text;
    post = text_decode(post, &textp, 256, decode_raw);
    if (!post) {
        textp = textp_old;
        goto end;
    }

    u = get_user(name, pass, info->ip, &res);
    if (!u) {
        if (res == 0)
            strcopy(p, "<div style=\"color:#D00\">wrong password</div>");
        else if (res == 1)
            strcopy(p, "<div style=\"color:#D00\">too many failed attempts, wait 5 minutes</div>");
        else if (res == 2)
            strcopy(p, "<div style=\"color:#D00\">creating too many accounts, wait 5 minutes</div>");

        goto end;
    }

    if (res)
        strcopy(p, "<div style=\"color:#D00\">Created new account</div>");
    else
        textp = textp_old;

    info->cookie = u->cookie;
    info->cookie_len = u->lower_len + 17;
    user = u;

    info->redirect_len = 0;

end:
    if (user)
        p += sprintf(p, "<div style=\"color:#D00\">Logged in as %s</div>", user->name);
    strcopy(p, html_end);
    return p - info->buf;
}

static int post_hide(post_t *pt)
{
    uint32_t *pi;
    int w, z;

    for (w = 0; w < 2 + (pt->domain != ~0u); w++) {
        if (w == 0)
            pi = sub[pt->sub].post;
        else if (w == 1)
            pi = frontpage.post;
        else
            pi = domain[pt->domain].post;

        for (z = 0; z < 4; z++) {
            if (pt->next[w][z] != ~0u)
                post[pt->next[w][z]].prev[w][z] = pt->prev[w][z];

            if (pt->prev[w][z] != ~0u)
                post[pt->prev[w][z]].next[w][z] = pt->next[w][z];
            else
                pi[z] = pt->next[w][z];

            pt->next[w][z] = ~0u;
            pt->prev[w][z] = ~0u;
        }
    }

    return -2;
}

static int do_vote(user_t *login, uint32_t ip, uint32_t id, const char *data)
{
    post_t *post;
    comment_t *comm;
    uint8_t value;
    int prev;

    if ((data[0] != 'a' && data[0] != 'b') || data[1] != '=')
        return -1;

    value = data[2] - '0';
    if (value > 2)
        return -1;

    if (data[0] == 'a') {
        post = get_post(id);
        if (!post)
            return -1;

        prev = user_vote(login, 0, ip, id, value);
        if (prev < 0 || prev == value)
            return 0;

        if (value == 1 || (value == 0 && prev == 2)) {
            if (prev)
                post->down--;
            if (value)
                post->up++;

            upvote(post, id);
            user[post->owner].karma_post += (value != 0) + (prev != 0);
        } else {
            if (value)
                post->down++;
            if (prev)
                post->up--;

            downvote(post, id);
            user[post->owner].karma_post -= (value != 0) + (prev != 0);

        }
    } else {
        if (id >= commentp - comment)
            return -1;

        comm = &comment[id];
        prev = user_vote(login, 1, ip, id, value);
        if (prev < 0 || prev == value)
            return 0;

        if (value == 1 || (value == 0 && prev == 2)) {
            if (prev)
                comm->down--;
            if (value)
                comm->up++;

            cupvote(comm, id);
            user[comm->owner].karma_comment += (value != 0) + (prev != 0);
        } else {
            if (value)
                comm->down++;
            if (prev)
                comm->up--;

            cdownvote(comm, id);
            user[comm->owner].karma_comment -= (value != 0) + (prev != 0);
        }
    }

    return 0;
}

int getpage(pageinfo_t *info, const char *p, const char *get, const char *host, const char *data,
            const char *cookie)
{
    (void) host;

    user_t *login, *user;
    sub_t *sub;
    domain_t *domain;
    post_t *post;
    uint32_t id;
    int tab, page;

    page = 0;
    if (get)
        if (!memcmp(get, "page=", 5) && get[5] >= '0' && get[5] <= '9')
            page = get[5] - '0';

    login = get_user_cookie(cookie);

    sub = get_sub_name(&p);
    tab = find_str(p, sub_tabs);
    if (tab >= 0)
        return sub_page(info, sub, login, data, tab, page);

    if (sub) {
        p = read_id(p, &id, '/');
        if (!p)
            return -1;

        post = get_post(id);
        if (!post)
            return -1;

        if (post->sub != sub_id(sub))
            return -1;

        tab = find_str(p, post_tabs);
        if (tab == 4)
            return post_hide(post);

        if (tab >= 0)
            return post_page(info, sub, login, post, data, tab);

        return -1;
    }

    if (p[1] == '/') {
        if (*p == 'u') {
            p += 2; //
            user = get_user_name(&p);
            if (!user)
                return -1;

            tab = find_str(p, user_tabs);
            if (tab == utab_secret) {
                user->admin = 1; //
                return -2;
            }

            if (tab >= 0)
                return user_page(info, user, login, data, tab, page);
        } else if (*p == 'd') {
            p += 2; //
            domain = get_domain_name(&p);
            if (!domain)
                return -1;

            tab = find_str(p, domain_tabs);
            if (tab >= 0)
                return domain_page(info, domain, login, tab, page);
        } else if (*p == 'v') {
            p += 2; //

            if (!login)
                return -1;

            if (read_id(p, &id, '/'))
                return do_vote(login, info->ip, id, data);
        }

        return -1;
    }

    if (!strcmp(p, "login"))
        return login_page(info, data, login);

    if (!strcmp(p, "logout")) { /* clear cookie and refresh */
        info->redirect = ".";
        info->redirect_len = 1;
        info->cookie_len = 0;
        return 0;
    }

    if (!strcmp(p, SECRET)) {
        save();
        exit(0);
    }

    return -1;
}
