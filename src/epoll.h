/* epoll.h:
 * epoll
 */

#ifndef __XM_EPOLL__
#define __XM_EPOLL__

#include <sys/epoll.h>
#include "http.h"
#include "threadpool.h"

#define MAXEVENTS 1024

int xm_epoll_create(int flags);

int xm_epoll_add(int epoll_fd, int fd, xm_http_request_t *request, int events);

int xm_epoll_mod(int epoll_fd, int fd, xm_http_request_t *request, int events);

int xm_epoll_del(int epoll_fd, int fd, xm_http_request_t *request, int events);

int xm_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout);

void xm_handle_events(int epoll_fd, int listen_fd, struct epoll_event* events,
                    int events_num, char *path, xm_threadpool_t *tp);

#endif
