/* priority_queue.c:
 * priority queue with comp
 */

#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"

// exchange two queues
void exch(xm_pq_t *xm_pq, size_t i, size_t j) {
    void *tmp = xm_pq->pq[i];
    xm_pq->pq[i] = xm_pq->pq[j];
    xm_pq->pq[j] = tmp;
}

// uppass the child
void swim(xm_pq_t *xm_pq, size_t k) {
    while (k > 1 && xm_pq->comp(xm_pq->pq[k], xm_pq->pq[k/2])) {
        exch(xm_pq, k, k/2);
        k /= 2;
    }
}

// downpass the child
int sink(xm_pq_t *xm_pq, size_t k) {
    size_t j;
    size_t nalloc = xm_pq->nalloc;
    while ((k << 1) <= nalloc) {
        j = k << 1;
        if ((j < nalloc) && (xm_pq->comp(xm_pq->pq[j+1], xm_pq->pq[j])))
            j++;

        if (!xm_pq->comp(xm_pq->pq[j], xm_pq->pq[k]))
            break;

        exch(xm_pq, j, k);
        k = j;
    }
    return k;
}

int xm_pq_sink(xm_pq_t *xm_pq, size_t i) {
    return sink(xm_pq, i);
}

int xm_pq_init(xm_pq_t *xm_pq, xm_pq_comparator_pt comp, size_t size) {
    // alloc for pq
    xm_pq->pq = (void **)malloc(sizeof(void *)*(size+1));
    if (!xm_pq->pq) {
        return -1;
    }

    xm_pq->nalloc = 0;
    xm_pq->size = size + 1;
    xm_pq->comp = comp;
    return 0;
}

int xm_pq_is_empty(xm_pq_t *xm_pq) {
    // judge whether it's empty nalloc
    return (xm_pq->nalloc == 0) ? 1 : 0;
}

size_t xm_pq_size(xm_pq_t *xm_pq) {
    // get size of priority queue
    return xm_pq->nalloc;
}

void *xm_pq_min(xm_pq_t *xm_pq) {
    // priority queue return minimum (pointer)
    if (xm_pq_is_empty(xm_pq))
        return (void *)(-1);

    return xm_pq->pq[1];
}

int resize(xm_pq_t *xm_pq, size_t new_size) {
    if (new_size <= xm_pq->nalloc)
        return -1;

    void **new_ptr = (void **)malloc(sizeof(void *)*new_size); 
    if (!new_ptr)
        return -1;
    // copy nalloc+1 element
    memcpy(new_ptr, xm_pq->pq, sizeof(void *)*(xm_pq->nalloc+1));
    free(xm_pq->pq);
    xm_pq->pq = new_ptr;
    xm_pq->size = new_size;
    return 0;
}

int xm_pq_delmin(xm_pq_t *xm_pq) {
    if (xm_pq_is_empty(xm_pq))
        return 0;

    exch(xm_pq, 1, xm_pq->nalloc);
    --xm_pq->nalloc;
    sink(xm_pq, 1);
    if ((xm_pq->nalloc > 0) && (xm_pq->nalloc <= (xm_pq->size-1)/4)) {
        if (resize(xm_pq, xm_pq->size / 2) < 0)
            return -1;
    }
    return 0;
}

int xm_pq_insert(xm_pq_t *xm_pq, void *item) {
    if (xm_pq->nalloc + 1 == xm_pq->size) {
        if (resize(xm_pq, xm_pq->size*2) < 0) {
            return -1;
        }
    }
    xm_pq->pq[++xm_pq->nalloc] = item;
    swim(xm_pq, xm_pq->nalloc);
    return 0;
}
