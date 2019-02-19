/* util.c:
 * 
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "util.h"
#include "http_request.h"
#include "epoll.h"

int read_conf(char *filename, xm_conf_t *conf) {
    // read only
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return XM_CONF_ERROR;

    char buff[BUFLEN];
    int buff_len = BUFLEN;
    char *curr_pos = buff;
    char *delim_pos = NULL;
    int i = 0;
    int pos = 0;
    int line_len = 0;
    while (fgets(curr_pos, buff_len - pos, fp)) {
        // locate the first delimiter for each line
        delim_pos = strstr(curr_pos, DELIM);
        if (!delim_pos)
            return XM_CONF_ERROR;
        if (curr_pos[strlen(curr_pos) - 1] == '\n') {
            curr_pos[strlen(curr_pos) - 1] = '\0';
        }

        // get root info
        if (strncmp("root", curr_pos, 4) == 0) {
            delim_pos = delim_pos + 1;
            while (*delim_pos != '#') {
                conf->root[i++] = *delim_pos;
                ++delim_pos;
            }
        }

        // get port value
        if (strncmp("port", curr_pos, 4) == 0)
            conf->port = atoi(delim_pos + 1);

        // get thread number
        if (strncmp("thread_num", curr_pos, 9) == 0)
            conf->thread_num = atoi(delim_pos + 1);

        // get line length
        line_len = strlen(curr_pos);

        // skip to next line
        curr_pos += line_len;
    }
    fclose(fp);
    return XM_CONF_OK;
}

void handle_for_sigpipe() { // do not use default SIGPIPE which may terminate process
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL))
        return;
}

int socket_bind_listen(int port) {
    // check port value
    port = ((port <= 1024) || (port >= 65535)) ? 6666 : port;

    // create socket (IPv4 + TCP), return listen fd
    int listen_fd = 0;
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    // get rid of "Address already in use" error
    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) == -1) {
        return -1;
    }

    // set server IP and Port, and bind listen fd
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY : 0.0.0.0 means unclear or any port
    server_addr.sin_port = htons((unsigned short) port);
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // start listening, wait queue length is LISTENQ
    if (listen(listen_fd, LISTENQ) == -1)
        return -1;

    // invalid listen fd
    if (listen_fd == -1) {
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int make_socket_non_blocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == 1)
        return -1;

    return 0;
}

void accept_connection(int listen_fd, int epoll_fd, char *path) {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = 0;
    int accept_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (accept_fd == -1)
        perror("accept");

    // set to non-blocking mode
    int rc = make_socket_non_blocking(accept_fd);

    // apply for xm_http_request_t and initialize
    xm_http_request_t *request = (xm_http_request_t *)malloc(sizeof(xm_http_request_t));
    xm_init_request_t(request, accept_fd, epoll_fd, path);

    // fd : readable, edge triggered, only one thread for one socket
    xm_epoll_add(epoll_fd, accept_fd, request, (EPOLLIN | EPOLLET | EPOLLONESHOT));

    // add time info
    xm_add_timer(request, TIMEOUT_DEFAULT, xm_http_close_conn);
}
