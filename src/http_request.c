/* http_request.c
 *
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "http_request.h"

static int xm_http_process_ignore(xm_http_request_t* request, xm_http_out_t* out, char* data, int len);

static int xm_http_process_connection(xm_http_request_t* request, xm_http_out_t* out, char* data, int len);

static int xm_http_process_if_modified_since(xm_http_request_t* request, xm_http_out_t* out, char* data, int len);

xm_http_header_handle_t xm_http_headers_in[] = {
    {"Host", xm_http_process_ignore},
    {"Connection", xm_http_process_connection},
    {"If-Modified-Since",xm_http_process_if_modified_since},
    {"", xm_http_process_ignore}
};

static int xm_http_process_ignore(xm_http_request_t* request, xm_http_out_t* out, char* data, int len){
    (void) request;
    (void) out;
    (void) data;
    (void) len;
    return 0;
}


// connnection style
static int xm_http_process_connection(xm_http_request_t* request, xm_http_out_t* out, char* data, int len){
    (void) request;
    // record whether it is a keep-alive connection
    if (strncasecmp("keep-alive", data, len) == 0) {
        out->keep_alive = 1;
    }
    return 0;
}

// judge whether the file was modified
static int xm_http_process_if_modified_since(xm_http_request_t* request, xm_http_out_t* out, char *data, int len){
    (void) request;
    (void) len;
    struct tm tm;
    // translate data into GMT Time
    if(strptime(data, "%a, %d %b %Y %H:%M:%S GMT", &tm) == (char*)NULL){
        return 0;
    }
    // translate time into seconds from 1970/01/01
    time_t client_time = mktime(&tm);
    // calculate time difference
    double time_diff = difftime(out->mtime, client_time);
    // us time : no modification
    if(fabs(time_diff) < 1e-6){
        out->modified = 0;
        out->status = XM_HTTP_NOT_MODIFIED;
    }
    return 0;
}

// initialize http_request
int xm_init_request_t(xm_http_request_t* request, int fd, int epoll_fd, char* path){
    request->fd = fd;
    request->epoll_fd = epoll_fd;
    request->pos = 0;
    request->last = 0;
    request->state = 0;
    request->root = path;
    INIT_LIST_HEAD(&(request->list));
    return 0;
}

// initialize out
int xm_init_out_t(xm_http_out_t* out, int fd){
    out->fd = fd;
    // default : 1, keep connected
    out->keep_alive = 1;
    out->modified = 1;
    // default : 200 (success)
    out->status = 200;
    return 0;
}

void xm_http_handle_header(xm_http_request_t* request, xm_http_out_t* out){
    list_head* pos;
    xm_http_header_t* hd;
    xm_http_header_handle_t* header_in;
    int len;
    list_for_each(pos, &(request->list)){
        hd = list_entry(pos, xm_http_header_t, list);
        for(header_in = xm_http_headers_in; strlen(header_in->name) > 0; header_in++){
            if(strncmp(hd->key_start, header_in->name, hd->key_end - hd->key_start) == 0){
                len = hd->value_end - hd->value_start;
                (*(header_in->handler))(request, out, hd->value_start, len);
                break;
            }    
        }
        list_del(pos);
        free(hd);
    }
}

// return shortmsg based on status
const char* get_shortmsg_from_status_code(int status_code){
    if(status_code == XM_HTTP_OK){
        return "OK";
    }
    if(status_code == XM_HTTP_NOT_MODIFIED){
        return "Not Modified";
    }
    if(status_code == XM_HTTP_NOT_FOUND){
        return "Not Found";
    }
    return "Unknown";
}

// close fd and free
int xm_http_close_conn(xm_http_request_t* request){
    close(request->fd);
    free(request);
    return 0;
}
