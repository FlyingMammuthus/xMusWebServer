/* http_request.h:
 * 
 */

#ifndef __HTTP_REQUEST__
#define __HTTP_REQUEST__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "list.h"
//#include "util.h"

#define XM_AGAIN EAGAIN

#define XM_HTTP_PARSE_INVALID_METHOD    10
#define XM_HTTP_PARSE_INVALID_REQUEST   11
#define XM_HTTP_PARSE_INVALID_HEADER    12

#define XM_HTTP_UNKNOWN         0x0001
#define XM_HTTP_GET             0x0002
#define XM_HTTP_HEAD            0x0004
#define XM_HTTP_POST            0x0008

#define XM_HTTP_OK              200
#define XM_HTTP_NOT_MODIFIED    304
#define XM_HTTP_NOT_FOUND       404
#define MAX_BUF 8124

typedef struct xm_http_request {
    char *root;
    int fd;
    int epoll_fd;
    char buff[MAX_BUF];
    size_t pos;
    size_t last;
    int state;

    void *request_start;
    void *method_end;
    int method;
    void *uri_start;
    void *uri_end;
    void *path_start;
    void *path_end;
    void *query_start;
    void *query_end;
    int http_major;
    int http_minor;
    void *request_end;

    struct list_head list;  // see list.h

    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;
    void *timer;
}xm_http_request_t;

typedef struct xm_http_out {
    int fd;
    int keep_alive;
    time_t mtime;
    int modified;
    int status;
}xm_http_out_t;

typedef struct xm_http_header {
    void *key_start;
    void *key_end;
    void *value_start;
    void *value_end;
    struct list_head list;
}xm_http_header_t;

typedef int (*xm_http_header_handler_pt)(xm_http_request_t *request, xm_http_out_t *out, char *data, int len);

typedef struct xm_http_header_handle {
    char *name;
    xm_http_header_handler_pt handler;
} xm_http_header_handle_t;

extern xm_http_header_handle_t xm_http_headers_in[];

void xm_http_handle_header(xm_http_request_t *request, xm_http_out_t *out);

int xm_http_close_conn(xm_http_request_t *request);

int xm_init_request_t(xm_http_request_t *request, int fd, int epoll_fd, char *path);

int xm_init_out_t(xm_http_out_t *out, int fd);

const char *get_shortmsg_from_status_code(int status_code);

#endif
