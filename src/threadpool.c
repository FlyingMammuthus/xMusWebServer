/* threadpool.c
 * thread pool
 */

#include "threadpool.h"

static int threadpool_free(xm_threadpool_t *pool);
static void *threadpool_worker(void *arg);

// free thread pool
int threadpool_free(xm_threadpool_t *pool) {
    if (pool == NULL || pool->started > 0)
        return -1;
    
    // free thread array
    if (pool->threads)
        free(pool->threads);

    // free task list
    xm_task_t *old;
    while (pool->head->next) {
        old = pool->head->next;
        pool->head->next = pool->head->next->next;
        free(old);
    }
    return 0;
}

void *threadpool_worker(void *arg) {
    if (arg == NULL)
        return NULL;

    xm_threadpool_t *pool = (xm_threadpool_t *)arg;
    xm_task_t *task;
    while (1) {
        // lock pool
        pthread_mutex_lock(&(pool->lock));

        // no task and not shutdown -> keep waiting
        while ((pool->queue_size == 0) && !(pool->shutdown))
            pthread_cond_wait(&(pool->cond), &(pool->lock));

        // immediate shutdown or graceful shutdown without task left
        if (pool->shutdown == immediate_shutdown)
            break;
        else if ((pool->shutdown == graceful_shutdown) && (pool->queue_size == 0))
            break;

        // get the first task
        task = pool->head->next;
        // no task, open locker and goto next while operation
        if (task == NULL) {
            pthread_mutex_unlock(&(pool->lock));
            continue;
        }

        // task fetching and open locker
        pool->head->next = task->next;
        pool->queue_size--;
        pthread_mutex_unlock(&(pool->lock));

        // set task func
        (*(task->func))(task->arg);
        free(task);
    }
    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return NULL;
}

// free thread
int threadpool_destroy(xm_threadpool_t *pool, int graceful) {
    if (pool == NULL)
        return xm_tp_invalid;
    if (pthread_mutex_lock(&(pool->lock)) != 0)
        return xm_tp_lock_fail;

    int err = 0;
    do {
        if (pool->shutdown) {
            err = xm_tp_already_shutdown;
            break;
        }

        pool->shutdown = (graceful) ? graceful_shutdown : immediate_shutdown;

        if (pthread_cond_broadcast(&(pool->cond)) != 0) {
            err = xm_tp_cond_broadcast;
            break;
        }

        if (pthread_mutex_unlock(&(pool->lock)) != 0) {
            err = xm_tp_lock_fail;
            break;
        }

        // free every thread
        for (int i = 0; i < pool->thread_count; ++i) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = xm_tp_thread_fail;
            }
        }
    }while (0);

    if (!err) {
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond));
        threadpool_free(pool);
    }
    return err;
}

// initialize thread pool
xm_threadpool_t *threadpool_init(int thread_num) {
    // alloc thread pool
    xm_threadpool_t *pool;
    if ((pool = (xm_threadpool_t *)malloc(sizeof(xm_threadpool_t))) == NULL)
        goto err;

    // threads pointer to thread array (for tid)
    pool->thread_count = 0;
    pool->queue_size = 0;
    pool->shutdown = 0;
    pool->started = 0;
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t)*thread_num);

    // alloc and initialize head pointer for task
    pool->head = (xm_task_t *)malloc(sizeof(xm_task_t));
    if ((pool->threads == NULL) || (pool->head == NULL))
        goto err;
    pool->head->func = NULL;
    pool->head->arg = NULL;
    pool->head->next = NULL;

    // initialize mutex
    if (pthread_mutex_init(&(pool->lock), NULL) != 0)
        goto err;

    // initialize condition 
    if (pthread_cond_init(&(pool->cond), NULL) != 0)
        goto err;

    // construct thread
    for (int i = 0; i < thread_num; ++i) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }
    return pool;

err:
    if (pool)
        threadpool_free(pool);
    return NULL;
}

int threadpool_add(xm_threadpool_t *pool, void (*func)(void *), void *arg) {
    int rc, err = 0;
    if (pool == NULL || func == NULL)
        return -1;

    if (pthread_mutex_lock(&(pool->lock)) != 0)
        return -1;

    // shutdown has been set
    if (pool->shutdown) {
        err = xm_tp_already_shutdown;
        goto out;
    }

    // create a new task and regestrate
    xm_task_t *task = (xm_task_t *)malloc(sizeof(xm_task_t));
    if (task == NULL)
        goto out;
    task->func = func;
    task->arg = arg;

    // create task node and insert
    task->next = pool->head->next;
    pool->head->next = task;
    pool->queue_size++;

    rc = pthread_cond_signal(&(pool->cond));

out:
    if (pthread_mutex_unlock(&pool->lock) != 0)
        return -1;
    return err;
}
