/* http server
*/

#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define __USE_GNU /* required for accept4() */
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>

/* functions to call */
int getpage(pageinfo_t *info, const char *p, const char *get, const char *host, const char *data,
            const char *cookie);
void time_event(void);
void init(void);

#ifndef PORT
#define PORT 80
#endif

#define TIMEOUT 5 /* connections are closed between TIMEOUT and TIMEOUT*2 seconds after accept */
#define POST_MAX 0x10000 /* maximum size of POST requests */

typedef struct {
    uint32_t ip;
    int sock, dlen, sent;
    char *data;
    int padding[2];
} client_t;

typedef struct {
    uint16_t family, port;
    uint32_t ip;
    uint8_t padding[8];
} addr_t;

/* http headers */
#define error_html(a, b) "HTTP/1.0 " a "\r\nContent-type: text/html\r\n\r\n" \
    "<html><head><title>" a " " b "</title></head>" \
    "<body bgcolor=\"white\"><center><h1>" a " " b "</h1></center><hr></body></html>" \

static const char* error[] = {
    error_html("404", "Not Found"),
    error_html("502", "Bad Gateway")
};

static int error_length[] = {
    sizeof(error_html("404", "Not Found")) - 1,
    sizeof(error_html("502", "Bad Gateway")) - 1
};

#define HEADER(x) "HTTP/1.0 200\r\nContent-type: " x "\r\n"
#define HLEN(x) (sizeof(HEADER(x)) - 1)

static const char* header[] = {
    HEADER("text/html;charset=utf-8"), HEADER("image/png"), HEADER("application/octet-stream")
};

static int header_length[] = {
    HLEN("text/html;charset=utf-8"), HLEN("image/png"), HLEN("application/octet-stream")
};

static const char redirect[] = "HTTP/1.0 302 Found\r\nLocation: ";

/* static data */
static client_t cl_list[1024 * 1024 * 2];
static pageinfo_t info;

static void client_free(client_t *cl)
{
    close(cl->sock);
    free(cl->data);
    cl->sock = -1;
}

/* handle http request
    HTTP header must always be complete (single read), but POST data can be incomplete
     *parses the whole header every time there is new POST data
*/

#define cmp(p, str) (!memcmp(*(p), str, sizeof(str) - 1) && (*(p) += sizeof(str) - 1, 1))
static void do_request(client_t *cl)
{
    int len, res, content_length;
    char *p, *path, *get, *host, *cookie, *real_ip, *content;
    bool post;

    cl->data[cl->dlen] = 0; /* work in null-terminated space */
    p = cl->data;

    if (!memcmp(p, "GET ", 4))
        post = 0;
    else if (!memcmp(p, "POST ", 5))
        post = 1;
    else
        goto invalid;

    p += 4 + post;
    if (*p++ != '/')
        goto invalid;

    /* parse requested path */
    path = p;
    get = 0;
    for (; *p != ' '; p++) {
        if (!*p)
            goto invalid;
        if (*p == '?')
            *p = 0, get = p + 1;
    }
    *p++ = 0; /* null-terminate the path */

    /* parse rest of header */
    host = 0;
    cookie = 0;
    real_ip = 0;
    content_length = -1;
    do {
        /* go to start of next line*/
        do {
            if (!*p)
                goto invalid;
        } while (memcmp(p, "\r\n", 2) && (p++, 1));
        *p = 0; p += 2;

        /* host */
        if (cmp(&p, "Host: ")) {
            host = p;
            continue;
        }

        /* cookie */
        if (cmp(&p, "Cookie: name=")) {
            cookie = p;
            continue;
        }

        /* content-length */
        if (cmp(&p, "Content-Length: ")) {
            content_length = strtoul(p, &p, 0);
            continue;
        }

        /* real-ip */
        if (cmp(&p, "X-Real-IP: ")) {
            real_ip = p;
            continue;
        }

        if (cmp(&p, "\r\n"))
            break;

        p++;
    } while (1); /* stop when line is empty */

    content = 0;
    if (content_length > 0) {
        if (!post || content_length > POST_MAX)
            goto invalid;

        /* check content */
        content = p;
        len = (cl->data + cl->dlen) - content;
        if (len < content_length) /* wait for more data */
            return;
    }

    info.ip = cl->ip;
    if ((info.ip & 0xFF) == 0x7F && real_ip)
        inet_pton(AF_INET, real_ip, &info.ip);

    /* respond */
    info.data = NULL;
    info.type = TEXT_HTML;
    info.cookie_len = -1;
	info.redirect_len = -1;
    len = getpage(&info, path, get, host, content, cookie);
    if (len < 0) {
        send(cl->sock, error[~len], error_length[~len], 0);
    } else {
        if (info.redirect_len >= 0) {
            send(cl->sock, redirect, sizeof(redirect) - 1, 0);
            send(cl->sock, info.redirect, info.redirect_len, 0);
            send(cl->sock, "\r\n", 2, 0);
        } else {
            send(cl->sock, header[info.type], header_length[info.type], 0);
        }

        if (info.cookie_len >= 0) {
            send(cl->sock, "Set-Cookie: name=", sizeof("Set-Cookie: name=") - 1, 0);
            send(cl->sock, info.cookie, info.cookie_len, 0);
            send(cl->sock, "\r\n", 2, 0);
        }

        send(cl->sock, "\r\n", 2, 0);

        res = send(cl->sock, (info.data ? info.data : info.buf), len, 0);
        if (res < len && res >= 0) { /* didnt send all data but no error, send rest later */
            len -= res;
            cl->dlen = -len;
            cl->sent = 0;
            cl->data = realloc(cl->data, len);
            if (cl->data) {
                memcpy(cl->data, (info.data ? info.data : info.buf) + res, len);
                free(info.data);
                return;
            }
        }
        free(info.data);
    }

invalid:
    client_free(cl);
}

static int tcp_init(const addr_t *addr)
{
    int sock, r, one;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        return -1;

    one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&one, sizeof(int));

    r = bind(sock, (const struct sockaddr*)addr, sizeof(*addr));
    if (r < 0)
        goto fail;

    r = listen(sock, SOMAXCONN);
    if(r < 0)
        goto fail;

    return sock;
fail:
    close(sock);
    return -1;
}

int main(void)
{
    const struct itimerspec itimer = {
        .it_interval = {.tv_sec = TIMEOUT}, .it_value = {.tv_sec = TIMEOUT},
    };

    addr_t addr = {
        .family = AF_INET,
        .port = __bswap_constant_16(PORT),
    };

    int efd, tfd, sock, client, nevent, n, len;
    struct epoll_event events[16], *ev;
    socklen_t addrlen;
    uint64_t exp;
    char *data;

    client_t *cl;
    int cl_count[2];
    bool list, timerevent;

    sock = tcp_init(&addr);
    if(sock < 0)
        return 1;

    tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(tfd < 0)
        goto exit_close_sock;

    timerfd_settime(tfd, 0, &itimer, NULL);

    efd = epoll_create(1);
    if(efd < 0)
        goto exit_close_tfd;

    ev = &events[0];
    ev->events = EPOLLIN;
    ev->data.u32 = -1;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, ev))
        goto exit_close_efd;

    ev->events = EPOLLIN;
    ev->data.u32 = -2;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, tfd, ev))
        goto exit_close_efd;

    list = 0;
    cl_count[0] = 0;
    cl_count[1] = 0;
    timerevent = 0;
    init();

    do {
        nevent = epoll_wait(efd, events, 16, -1);
        if(nevent < 0) { /* happens sometimes ?? */
            printf("epoll error %i\n", errno);
            continue;
        }

        if(nevent == 0) { /* should never happen */
            printf("epoll_wait returned 0\n");
            break;
        }

        ev = events;
        do {
            n = ev->data.u32;
            if(n < 0) {
                if(n == -1) {
                    addrlen = sizeof(addr);
                    client = accept4(sock, (struct sockaddr*)&addr, &addrlen, SOCK_NONBLOCK);
                    if (client < 0) {
                        printf("accept failed %i\n", errno);
                        continue;
                    }

                    if (cl_count[list] == 1024 * 1024)
                        continue;

                    n = list << 20 | cl_count[list]++;
                    cl = &cl_list[n];
                    cl->sock = client;
                    cl->dlen = 0;
                    cl->data = NULL;
                    cl->ip = addr.ip;

                    ev->events = (EPOLLIN | EPOLLOUT) | EPOLLET;
                    ev->data.u32 = n;
                    epoll_ctl(efd, EPOLL_CTL_ADD, client, ev);
                } else { /* timerfd event */
                    n = read(tfd, &exp, 8);
                    timerevent = 1;
                }
            } else { /* client socket event */
                cl = &cl_list[n];
                if(cl->dlen < 0) { /* waiting for EPOLLOUT */
                    if (!(ev->events & EPOLLOUT))
                        continue;

                    n = -cl->dlen;
                    len = send(cl->sock, cl->data + cl->sent, n, 0);
                    if(len < 0) {
                        client_free(cl);
                        continue;
                    }
                    cl->sent += len;
                    cl->dlen += len;
                    if(cl->dlen == 0)
                        client_free(cl);
                    continue;
                }

                if(!(ev->events & EPOLLIN)) { /* no data available */
                    continue;
                }

                /* get bytes available */
                n = ioctl(cl->sock, FIONREAD, &len);
                if(n < 0) {
                    printf("ioctl error %u\n", errno);
                    client_free(cl);
                    continue;
                }

                if(cl->dlen + len > POST_MAX) {
                    client_free(cl);
                    continue;
                }

                data = realloc(cl->data, cl->dlen + len + 16);
                /* +1 for null terminator, +16 for safe memcmps */
                if(!data || recv(cl->sock, data + cl->dlen, len, 0) != len) {
                    client_free(cl);
                    continue;
                }

                cl->data = data;
                cl->dlen += len;
                do_request(cl);
            }
        } while(ev++, --nevent);

        /* process timer event only after all other events */
        if(timerevent) {
            timerevent = 0;
            time_event();

            list = !list;
            n = cl_count[list];
            cl_count[list] = 0;
            cl = &cl_list[list << 20];
            while (n--) {
                if(cl->sock >= 0)
                    client_free(cl); //does not need to set sock=-1
                cl++;
            }
        }
    } while(1);

exit_close_efd:
    close(efd);
exit_close_tfd:
    close(tfd);
exit_close_sock:
    close(sock);
    return 1;
}
