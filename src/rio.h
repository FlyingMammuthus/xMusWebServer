/* rio: 
    Robust Read and Write
    A wrapper for write and read function.
 */

#ifndef __ROBUST_IO__
#define __ROBUST_IO__

#include <sys/types.h>
#define RIO_BUFSIZE 8192

typedef struct {
    int rio_fd;                 // descriptor for this internal buf
    ssize_t rio_cnt;            // unreal bytes in internal nuf
    char *rio_bufptr;           // next unread byte in internal buf
    char rio_buf[RIO_BUFSIZE];  // internal buffer
} rio_t;

// robust read for n-size data
ssize_t rio_readn(int fd, void *usrbuf, size_t n);

// robust write for n-size write
ssize_t rio_writen(int fd, void *usrbuf, size_t n);

// initialize rio_t
void rio_readinitb(rio_t *rp, int fd);

// robust read for n-size data using buf
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);

// robust read for one-line data using buf
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

#endif

