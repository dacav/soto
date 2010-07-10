#include "headers/thrd.h"

#include <sched.h>
#include <error.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Flags for thrd_t::status */
#define THRD_ALIVE          1 << 0
#define THRD_INITIALIZED    1 << 2

/* Flags for thrd_pool_t::status */
#define THRD_POOL_LIB_ERR   1 << 0 /**< Library error */
#define THRD_POOL_ACTIVE    1 << 1 /**< Active pool (running threads) */
#define THRD_POOL_KILLALL   1 << 2 /**< All threads shutting down */

/* Test for the condition variable of thrd_sync_t */
#define THRD_POOL_CONDITION ( THRD_POOL_ACTIVE | THRD_POOL_KILLALL )

static
void * thread_routine (void * arg)
{
    thrd_t *thrd = (thrd_t *)arg;
    thrd_sync_t *sync = thrd->sync;
    thrd_info_t *info = thrd->info;
    uint8_t ctrl;

    for (;;) {

        pthread_mutex_lock(&sync->mux);
        while ((*sync->ctrl & THRD_POOL_CONDITION) == 0) {
            pthread_cond_wait(&sync->cond, &sync->mux);
        }
        ctrl = *sync->ctrl;
        pthread_mutex_unlock(&sync->mux);

        /* I've been asked to do seppuku. */
        if (ctrl & THRD_POOL_KILLALL) {
            DEBUG_MSG("Seppuku required");
            break;
        }

        /* Spontaneous seppuku */
        if (info->callback(info->context) == 0) {
            DEBUG_MSG("Completed my job");
            break;
        }

    }

    pthread_exit(NULL);
}

static
int startup(thrd_t *thrd, thrd_sync_t *sync, thrd_info_t *userinfo)
{
    pthread_attr_t attr;
    int err;
    struct sched_param param = {
        .sched_priority = userinfo->priority
    };

    /* Thread enabled but still not active, thus it will wait until
     * active. */
    thrd->sync = sync;
    thrd->info = userinfo;

    /* Setting thread as real-time, scheduled as FIFO and with the given
     * priority. */
    assert(pthread_attr_init(&attr) == 0);
    assert(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) == 0);
    assert(pthread_attr_setschedparam(&attr, &param) == 0);
    assert(pthread_attr_setinheritsched(&attr,
           PTHREAD_EXPLICIT_SCHED) == 0);
    err = pthread_create(&thrd->handler, &attr, thread_routine,
                         (void *) thrd);
    pthread_attr_destroy(&attr);
    if (err != 0) return err;

    DEBUG_FMT("Activated thread with id [%08X], priority [%d]",
              (unsigned) thrd->handler, param.sched_priority);
    thrd->status = THRD_INITIALIZED | THRD_ALIVE;
    return 0;
}

static
void sync_init (thrd_sync_t *sync, uint8_t *ctrl)
{
    pthread_cond_init(&sync->cond, NULL);
    pthread_mutex_init(&sync->mux, NULL);
    sync->ctrl = ctrl;
}

static
void sync_destroy (thrd_sync_t *sync)
{
    pthread_cond_destroy(&sync->cond);
    pthread_mutex_destroy(&sync->mux);
}

int thrd_init (thrd_pool_t *pool, dlist_t *threads, size_t nthreads)
{
    thrd_t *ths;
    diter_t *i;

    pool->err = 0;

    assert(ths = (thrd_t *) malloc(sizeof(thrd_t) * nthreads));
    pool->threads = ths;
    pool->nthreads = nthreads;

    sync_init(&pool->sync, &pool->status);

    i = dlist_iter_new(&threads);
    while (diter_hasnext(i)) {
        int err;

        err = startup(ths ++, &pool->sync, (thrd_info_t *) diter_next(i));
        if (err != 0) {
            pool->status |= THRD_POOL_LIB_ERR;
            pool->err = err;
            thrd_destroy(pool);
            return -1;
        }

    }
    dlist_iter_free(i);

    return 0;
}

void thrd_start (thrd_pool_t *pool)
{
    pthread_mutex_lock(&pool->sync.mux);
    pool->status |= THRD_POOL_ACTIVE;
    pthread_cond_broadcast(&pool->sync.cond);
    pthread_mutex_unlock(&pool->sync.mux);
}

void thrd_pause (thrd_pool_t *pool)
{
    pthread_mutex_lock(&pool->sync.mux);
    pool->status &= THRD_POOL_ACTIVE;
    pthread_mutex_unlock(&pool->sync.mux);
}

void thrd_destroy (thrd_pool_t *pool)
{
    size_t n = pool->nthreads;
    thrd_t *cur = pool->threads;

    pthread_mutex_lock(&pool->sync.mux);
    pool->status |= THRD_POOL_KILLALL;
    pthread_mutex_unlock(&pool->sync.mux);

    while (n--) {
        pthread_join(cur->handler, NULL);
    }
    free(pool->threads);

    sync_destroy(&pool->sync);
}

const char * thrd_strerr (thrd_pool_t *pool)
{
    if (pool->status & THRD_POOL_LIB_ERR)
        return strerror(pool->err);
    return "Unknown";
}

