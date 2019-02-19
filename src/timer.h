/* timer.h:
 * timer
 */

#ifndef __XM_TIMER__
#define __XM_TIMER__

#include "priority_queue.h"
#include "http_request.h"

#define TIMEOUT_DEFAULT 500 //ms

// function pointer for timeout
typedef int (*timer_handler_pt)(xm_http_request_t *request);

typedef struct xm_timer {
    size_t key;         // mark whether it is time out
    int deleted;        // mark whether it is deleted
    timer_handler_pt handler;   // handler function
    xm_http_request_t *request; // pointer to request
} xm_timer_t;

// xm_pq_t defined in "priority_queue.h"
extern xm_pq_t xm_timer;
extern size_t xm_current_msec;

int xm_timer_init();

int xm_find_timer();

void xm_handle_expire_timers();

void xm_add_timer(xm_http_request_t *request, size_t timeout, timer_handler_pt handler);

void xm_del_timer(xm_http_request_t *request);

int timer_comp(void *ti, void *tj);

#endif
