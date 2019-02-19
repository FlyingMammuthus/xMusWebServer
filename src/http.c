/* http.c:
 * http do request
 */

#include <errno.h>
#include "http.h"

static const char *get_file_type(const char *type);

static void parse_uri(char *uri, int length, char *filename, char *query);

static void do_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg);

static void serve_static(int fd, char *filename, size_t filesize, xm_http_out_t *out);

static char *ROOT = NULL;

mime_type_t xmus_mime[] = 
{
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/msword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css"},
    {NULL, "text/plain"}
};

static void parse_uri(char *uri_start, int uri_length, char *filename, char *query) {
    uri_start[uri_length] = '\0';
    // find '?' and locate the division for non-parameter
    char *delim_pos = strchr(uri_start, '?');
    int filename_length = (delim_pos != NULL) ? ((int)(delim_pos - uri_start)) : uri_length;
    strcpy(filename, ROOT);
    // insert content from filename until reaching '?'
    strncat(filename, uri_start, filename_length);
    // find out the last '/'
    char *last_comp = strrchr(filename, '/');   // strchr : find first      strrchr : find last 
    // find last '.' to get file type
    char *last_dot = strrchr(last_comp, '.');
    // add '/' to the end if no file type (directory)
    if ((last_dot == NULL) && (filename[strlen(filename) - 1] != '/')) {
        strcat(filename, "/");
    }
    // default request : index.html
    if (filename[strlen(filename) - 1] == '/') {
        strcat(filename, "index.html");
    }
    return;
}

const char *get_file_type(const char *type) {
    // compare type with xmus_mime list, and return file type to fill Content-Type
    for (int i = 0; xmus_mime[i].type != NULL; ++i) {
        if (strcmp(type, xmus_mime[i].type) == 0)
            return xmus_mime[i].value;
    }
    // no result 
    return "text/plain";
}

// Response error
void do_error(int fd, char *cause, char *err_num, char *short_msg, char *long_msg) {
    // reponse header (512 bytes) and data buffer (8192 bytes)
    char header[MAXLINE];
    char body[MAXLINE];

    // fill response error with log_msg and cause
    sprintf(body, "<html><title>xMus Error<title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\n", body);
    sprintf(body, "%s%s : %s\n", body, err_num, short_msg);
    sprintf(body, "%s<p>%s : %s\n</p>", body, long_msg, cause);
    sprintf(body, "%s<hr><em>xMus web server</em>\n</body></html>", body);

    // return error number, and create error response header
    sprintf(header, "HTTP/1.1 %s %s\r\n", err_num, short_msg);
    sprintf(header, "%sServer: xMus\r\n", header);
    sprintf(header, "%sContent-type: text/html\r\n", header);
    sprintf(header, "%sConnection: close\r\n", header);
    sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));

    // Add 404 Page

    // send error msg
    rio_writen(fd, header, strlen(header));
    rio_writen(fd, body, strlen(body));
    return;
}

// do static serve
void serve_static(int fd, char *filename, size_t filesize, xm_http_out_t *out) {
    // response header buffer (512 bytes) and data buffer (8192 bytes)
    char header[MAXLINE];
    char buff[SHORTLINE];
    struct tm tm;

    // return response header, including HTTP version and short msg of the stat number
    sprintf(header, "HTTP/1.1 %d %s\r\n", out->status, get_shortmsg_from_status_code(out->status));

    // return response header
    // Connection, Keep-Alive, Content-type, Content-length, Last_Modified
    if (out->keep_alive) {
        // return default connection type and alive time (default: 500 ms)
        sprintf(header, "%sConnection: keep-alive\r\n", header);
        sprintf(header, "%sKepp-Alive: timeout=%d\r\n", header, TIMEOUT_DEFAULT);
    }
    if (out->modified) {
        // get file type and fill Content-type
        const char *filetype = get_file_type(strrchr(filename, '.'));
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        // return file size from Content-length
        sprintf(header, "%sContent-length: %zu\r\n", header, filesize);
        // get time of last modification
        localtime_r(&(out->mtime), &tm);
        strftime(buff, SHORTLINE, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buff);
    }
    sprintf(header, "%sServer : xMus\r\n", header);

    // blank line (neccessary)
    sprintf(header, "%s\r\n", header);

    //send header and check completeness
    size_t send_len = (size_t)rio_writen(fd, header, strlen(header));
    if (send_len != strlen(header)) {
        perror("Send header failed!");
        return;
    }

    // open and send file
    int src_fd = open(filename, O_RDONLY, 0);
    char *src_addr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);

    // send file and check completeness
    send_len = rio_writen(fd, src_addr, filesize);
    if (send_len != filesize) {
        perror("Send file failed!");
        return;
    }
    munmap(src_addr, filesize);
}

int error_process(struct stat *sbufptr, char *filename, int fd) {
    // no file error
    if (stat(filename, sbufptr) < 0) {
        do_error(fd, filename, "404", "Not Found", "xMus cannot find the file");
        return 1;
    }

    // authority error
    if (!(S_ISREG((*sbufptr).st_mode)) || !(S_IRUSR & (*sbufptr).st_mode)) {
        do_error(fd, filename, "403", "Forbidden", "xMus cannot read the file");
        return 1;
    }

    return 0;
}

void do_request(void *ptr) {
    xm_http_request_t *request = (xm_http_request_t *)ptr;
    int fd = request->fd;
    ROOT = request->root;
    char filename[SHORTLINE];
    struct stat sbuf;
    int rc, n_read;
    char *plast = NULL;
    size_t remain_size;

    xm_del_timer(request);

    while(1) {
        // plast points to the first buf bytes ready for writing
        plast = &request->buff[request->last % MAX_BUF];

        // remain_size is the left bytes ready for writing
        remain_size = MIN(MAX_BUF - (request->last - request->pos) - 1, MAX_BUF - request->last % MAX_BUF);

        // read data from fd and copy to plast
        n_read = read(fd, plast, remain_size);

        // EOF or no data, disconnect
        if (n_read == 0)
            goto err;

        // non EAGAIN Error, disconnect
        if (n_read < 0 && (errno != XM_AGAIN))
            goto err;

        // non-blocking, return EAGAIN and reset timer, reqister again, keep alive and wait for next request
        if ((n_read < 0) && (errno == XM_AGAIN))
            break;

        // update read byte number 
        request->last += n_read;

        // parse request line
        rc = xm_http_parse_request_line(request);
        if (rc == XM_AGAIN)
            continue;
        else if (rc != 0)
            goto err;

        // parse request body
        rc = xm_http_parse_request_body(request);
        if (rc == XM_AGAIN)
            continue;
        else if (rc != 0)
            goto err;

        // distribute and initialize data structure for returning
        xm_http_out_t *out = (xm_http_out_t *)malloc(sizeof(xm_http_out_t));
        xm_init_out_t(out, fd);

        // parse uri and get file name
        parse_uri(request->uri_start, request->uri_end - request->uri_start, filename, NULL);

        // process error
        if (error_process(&sbuf, filename, fd))
            continue;

        xm_http_handle_header(request, out);

        // get requested file type
        out->mtime = sbuf.st_mtime;

        // process static serve request
        serve_static(fd, filename, sbuf.st_size, out);

        // free data
        free(out);

        // process HTTP long connection, TCP connect control 
        if (!out->keep_alive)
            goto close;
    }
    // one request ends, reset stat instead of disconnecting directly
    // revise event type of fd
    // reset timer and wait for next-time request
    xm_epoll_mod(request->epoll_fd, request->fd, request, (EPOLLIN | EPOLLET | EPOLLONESHOT));
    xm_add_timer(request, TIMEOUT_DEFAULT, xm_http_close_conn);

    // return and transfer worker thread access and keep alive and listening through epoll
    return;

    err:
    close:
    // error or normal close
    // clsoe and free
    xm_http_close_conn(request);
}
