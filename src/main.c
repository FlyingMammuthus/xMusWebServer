/* main.c:
 * start main()
 */

// head file
#include <stdio.h>
#include "threadpool.h"
#include "http.h"

#define DEFAULT_CONFIG "xMus.conf"

extern struct epoll_event *events;
char *conf_file = DEFAULT_CONFIG;
xm_conf_t conf;

int main(int argc, char *argv[]) {
    // read conf file
    read_conf(conf_file, &conf);

    // process SIGPIPE
    handle_for_sigpipe();

    // initialize socket and start listening
    int listen_fd = socket_bind_listen(conf.port);

    // set socket to non-blocking
    int rc = make_socket_non_blocking(listen_fd);

    // create epoll and register for listen_fd
    int epoll_fd = xm_epoll_create(0);
    xm_http_request_t *request = (xm_http_request_t *)malloc(sizeof(xm_http_request_t));
    xm_init_request_t(request, listen_fd, epoll_fd, conf.root);
    xm_epoll_add(epoll_fd, listen_fd, request, (EPOLLIN | EPOLLET));

    // initialize thread pool
    xm_threadpool_t *tp = threadpool_init(conf.thread_num);

    // initialize timer
    xm_timer_init();

    while (1) {
        // get the closest time
        int time = xm_find_timer();

        // call epoll_wait, and return event number
        int events_num = xm_epoll_wait(epoll_fd, events, MAXEVENTS, -1);

        // process time-out request
        xm_handle_expire_timers();

        // traverse events array, distribute ops based on fd
        xm_handle_events(epoll_fd, listen_fd, events, events_num, conf.root, tp);
    }

    // reclaim thread memory
    // threadpool_destroy(tp, graceful_shutdown);
}
