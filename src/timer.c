/* timer.c:
 * timer
 */

#include <sys/time.h>
#include "timer.h"

xm_pq_t xm_timer;
size_t xm_current_msec;

// ti < tj return 1 else 0
int timer_comp(void *ti, void *tj) {
    xm_timer_t *timeri = (xm_timer_t *)ti;
    xm_timer_t *timerj = (xm_timer_t *)tj;
    return (timeri->key < timerj->key) ? 1 : 0;
}

// get time (ms)
void xm_timer_update() {
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    xm_current_msec = ((tv.tv_sec*1000) + (tv.tv_usec/1000));
}

int xm_timer_init() {
    int rc = xm_pq_init(&xm_timer, timer_comp, XM_PQ_DEFAULT_SIZE);
    // update time 
    xm_timer_update();
    return 0;
}

int xm_find_timer() {
    int time = -1;
    // return time difference between time minimum in queue and present time
    while (!xm_pq_is_empty(&xm_timer)) {
        // update present time
        xm_timer_update();
        // timer_node points to time minimum 
        xm_timer_t *timer_node = (xm_timer_t *)xm_pq_min(&xm_timer);
        // if deleted, free
        if (timer_node->deleted) {
            int rc = xm_pq_delmin(&xm_timer);
            free(timer_node);
            continue;
        }
        // get time
        time = (int)(timer_node->key - xm_current_msec);
        time = (time > 0) ? time : 0;
        break;
    }
    return time;
}

void xm_handle_expire_timers() {
    while (!xm_pq_is_empty(&xm_timer)) {
        // update present time
        xm_timer_update();
        xm_timer_t *timer_node = (xm_timer_t *)xm_pq_min(&xm_timer);
        // if deleted, free
        if (timer_node->deleted) {
            int rc = xm_pq_delmin(&xm_timer);
            free(timer_node);
            continue;
        }
        // check expiration
        if (timer_node->key > xm_current_msec) { 
            return;       // no expiration
        }
        // expired, call handler
        if (timer_node->handler) {
            timer_node->handler(timer_node->request);
        }
        int rc = xm_pq_delmin(&xm_timer);
        free(timer_node);
    }
}

void xm_add_timer(xm_http_request_t *request, size_t timeout, timer_handler_pt handler) {
    xm_timer_update();
    // add new xm_timer_t node into xm_http_request_t timer
    xm_timer_t *timer_node = (xm_timer_t *)malloc(sizeof(xm_timer_t));
    request->timer = timer_node;
    // set threshold for expiration
    timer_node->key = xm_current_msec + timeout;
    timer_node->deleted = 0;
    timer_node->handler = handler;
    // set request for timer_node
    timer_node->request = request;
    // insert new node into priority queue
    int rc = xm_pq_insert(&xm_timer, timer_node);
}

void xm_del_timer(xm_http_request_t *request) {
    xm_timer_update();
    xm_timer_t *timer_node = request->timer;
    // lazy deletion
    timer_node->deleted = 1;
}

