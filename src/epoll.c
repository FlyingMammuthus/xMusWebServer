/* epoll.c:
 * define wrapping function with epoll
 */

#include "epoll.h"

struct epoll_event *events;

int xm_epoll_create(int flags) {
    int epoll_fd = epoll_create1(flags);
    if (epoll_fd == -1)
        return -1;
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    return epoll_fd;
}

// register for all new fd
int xm_epoll_add(int epoll_fd, int fd, xm_http_request_t *request, int events) {
    struct epoll_event event;
    event.data.ptr = (void *)request;
    event.events = events;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    if (ret == -1)
        return -1;
}

// revise fd
int xm_epoll_mod(int epoll_fd, int fd, xm_http_request_t *request, int events) {
    struct epoll_event event;
    event.data.ptr = (void *)request;
    event.events = events;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
    if (ret == -1)
        return -1;
}

// delete fd from epoll
int xm_epoll_del(int epoll_fd, int fd, xm_http_request_t *request, int events) {
    struct epoll_event event;
    event.data.ptr = (void *)request;
    event.events = events;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
    if (ret == -1)
        return -1;
}

// return active activity number
int xm_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout) {
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    return ret_count;
}

// handle events
void xm_handle_events(int epoll_fd, int listen_fd, struct epoll_event *events,
                    int events_num, char *path, xm_threadpool_t *tp) {
    for (int i = 0; i < events_num; ++i) {
        // get fd for events
        xm_http_request_t *request = (xm_http_request_t *)(events[i].data.ptr);
        int fd = request->fd;

        // listen_fd for events
        if (fd == listen_fd) {
            accept_connection(listen_fd, epoll_fd, path);
        }
        else{
            // connection for events

            // remove false events
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (!(events[i].events & EPOLLIN))) {
                close(fd);
                continue;
            }

        // add request into thread pool
        int rc = threadpool_add(tp, do_request, events[i].data.ptr);
        //do_request(events[i].data.ptr);
        }
    }
}
