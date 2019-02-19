/* util.h:
 *
 */

#ifndef __XM_UTIL__
#define __XM_UTIL__

#define PATHLEN 128
#define LISTENQ 1024
#define BUFLEN 8192
#define DELIM "="

#define XM_CONF_OK 0
#define XM_CONF_ERROR -1

#define MIN(a, b) ((a) < b ? (a):(b))

typedef struct xm_conf {
    char root[PATHLEN];
    int port;
    int thread_num;
}xm_conf_t;

int read_conf(char *filename, xm_conf_t *conf);

void handle_for_sigpipe();

int socket_bind_listen(int port);

int make_socket_non_blocking(int fd);

void accept_connection(int listen_fd, int epoll_fd, char *path);

#endif
