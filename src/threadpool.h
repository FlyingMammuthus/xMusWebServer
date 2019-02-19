/* threadpool.h
 * thread pool
 */

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

typedef struct xm_task {
    void (*func)(void*);
    void* arg;
    struct xm_task *next;   // next pointer in the list
}xm_task_t;

typedef struct threadpool {
    pthread_mutex_t lock;   // mutex lock
    pthread_cond_t  cond;   // condition
    pthread_t *threads;     // thread
    xm_task_t *head;        // task list
    int thread_count;       // thread number
    int queue_size;         // list length
    int shutdown;           // shutdown mode
    int started;
}xm_threadpool_t;

typedef enum {
    xm_tp_invalid = -1,
    xm_tp_lock_fail = -2,
    xm_tp_already_shutdown = -3,
    xm_tp_cond_broadcast = -4,
    xm_tp_thread_fail = -5
}xm_threadpool_error_t;

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
}xm_threadpool_sd_t;

xm_threadpool_t *threadpool_init(int thread_num);

int threadpool_add(xm_threadpool_t *pool, void(*func)(void *), void *arg);

int threadpool_destroy(xm_threadpool_t *pool, int graceful);

#endif
