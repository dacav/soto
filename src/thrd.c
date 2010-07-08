#include "headers/thrd.h"
#include "headers/options.h"

#include <sched.h>
#include <error.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

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

    DEBUG_FMT("Started and waiting, period=%llu",
              (unsigned long long) thrd->period);

    pthread_mutex_lock(&sync->mux);
    while ((*sync->ctrl & THRD_POOL_CONDITION) == 0) {
        pthread_cond_wait(&sync->cond, &sync->mux);
        DEBUG_FMT("Woken up period=%d", (int)thrd->period);
    }
    pthread_mutex_unlock(&sync->mux);

    DEBUG_FMT("Phase 1 period=%d", (int)thrd->period);
    uint16_t n = 0;
    for (n = 0; n < 30000; n ++);
    DEBUG_FMT("Phase 2 period=%d", (int)thrd->period);
    for (n = 0; n < 30000; n ++);
    DEBUG_FMT("Phase 3 period=%d", (int)thrd->period);

    return NULL;
}

static
int startup(thrd_t *thrd, thrd_sync_t *sync, int prio)
{
    pthread_attr_t attr;
    int err;
    struct sched_param param = {
        .sched_priority = prio
    };

    DEBUG_FMT("Enabling thread (period %3llu) with priority %d...",
          (long long unsigned) thrd->period, prio);

    /* Thread enabled but still not active, thus it will wait until
     * active. */
    thrd->sync = sync;

    /* Setting thread as real-time, scheduled as FIFO and with the given
     * priority. */
    assert(pthread_attr_init(&attr) == 0);
    assert(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) == 0);
    assert(pthread_attr_setschedparam(&attr, &param) == 0);
    assert(pthread_attr_setinheritsched(&attr,
           PTHREAD_EXPLICIT_SCHED) == 0);
    err = pthread_create(&thrd->handler, &attr, thread_routine,
                         (void *)thrd);
    pthread_attr_destroy(&attr);
    if (err != 0) return err;

    thrd->status = THRD_INITIALIZED | THRD_ALIVE;

    return err;
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

static
int cmp_period (const void *v0, const void *v1)
{
    const thrd_t *i0 = *((const thrd_t **) v0);
    const thrd_t *i1 = *((const thrd_t **) v1);

    /* If i0's period is smaller w.r.t i1's one, then i0 must come
     * first. */
    return i1->period - i0->period;
}

/* This function produces a heap which orders the threads by period:
 * shortest one come first */
static
void insert_ordered (dlist_t *tinfo, thrd_t **ths, size_t nt)
{
    size_t used = 0;
    diter_t *i;

    i = dlist_iter_new(&tinfo);
    while (diter_hasnext(i)) {
        thrd_t *t;

        assert(t = (thrd_t *) malloc(sizeof(thrd_t)));
        t->period = ((thrd_info_t *) diter_next(i))->period;
        ths[used ++] = t; 
    }
    dlist_iter_free(i);

    qsort((void *)ths, nt, sizeof(thrd_t *), cmp_period);
}

int thrd_init (thrd_pool_t *pool, opts_t *opts)
{
    size_t nt;
    thrd_t **ths;
    register int i;
    int prio;

    pool->err = 0;

    pool->nthreads = nt = opts->nthreads;
    assert(ths = (thrd_t **) malloc(sizeof(thrd_t *) * nt));
    pool->threads = ths;

    sync_init(&pool->sync, &pool->status);

    /* Ordering by thread periods */
    insert_ordered(opts->threads, ths, nt);

    prio = opts->minprio + 1;
    for (i = 0; i < nt; i ++) {
        int err;
        
        err = startup(ths[i], &pool->sync, prio + i);
        if (err != 0) {
            pool->status |= THRD_POOL_LIB_ERR;
            pool->err = err;
            thrd_destroy(pool);
            return -1;
        }
    }

    return 0;
}

void thrd_enable_switch (thrd_pool_t *pool)
{
    pthread_mutex_lock(&pool->sync.mux);
    pool->status ^= THRD_POOL_ACTIVE;
    pthread_mutex_unlock(&pool->sync.mux);

    pthread_cond_broadcast(&pool->sync.cond);
}

void thrd_destroy (thrd_pool_t *pool)
{
    int i;
    thrd_t **ths;

    // TODO: remember to KILLALL

    sync_destroy(&pool->sync);
    i = pool->nthreads;
    ths = pool->threads;
    while (i --) {
        if (ths[i]->status & THRD_INITIALIZED) {
            // TODO: wait for it, than destroy
        }
    }
    free(ths);
}

const char * thrd_strerr (thrd_pool_t *pool)
{
    if (pool->status & THRD_POOL_LIB_ERR)
        return strerror(pool->err);
    return "Unknown";
}

