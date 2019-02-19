/* http_parse.h
 * parse http request
 */

#ifndef __HTTP_PARSE__
#define __HTTP_PARSE__

#define CR '\r'
#define LF '\n'

// http request line parse
int xm_http_parse_request_line(xm_http_request_t *request);

// http request parse
int xm_http_parse_request_body(xm_http_request_t *request);

#endif
