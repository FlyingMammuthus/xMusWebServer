/* priority_queue.h:
 * priority queue
 */

#ifndef __PRIORITY_QUEUE__
#define __PRIORITY_QUEUE__

#include <stdlib.h>

#define XM_PQ_DEFAULT_SIZE 10

typedef int (*xm_pq_comparator_pt)(void *pi, void *pj);

typedef struct priority_queue {
    void **pq;          // pointer array whose elemnt points to data, pq[0] should be null
    size_t nalloc;      // number of data stored in the queue
    size_t size;        // maximum number of the queue (size)
    xm_pq_comparator_pt comp;
} xm_pq_t;

int xm_pq_init(xm_pq_t *xm_pq, xm_pq_comparator_pt comp, size_t size);

int xm_pq_is_empty(xm_pq_t *xm_pq);

size_t xm_pq_size(xm_pq_t *tk_pq);

void *xm_pq_min(xm_pq_t *xm_pq);

int xm_pq_delmin(xm_pq_t *xm_pq);

int xm_pq_insert(xm_pq_t *xm_pq, void *item);

int xm_pq_sink(xm_pq_t *xm_pq, size_t i);

#endif
