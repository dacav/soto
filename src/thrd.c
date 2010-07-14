#include "headers/thrd.h"
#include "headers/rtutils.h"

#include <sched.h>
#include <error.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* Flags for thrd_t::status */
#define THRD_ALIVE          1 << 0
#define THRD_INITIALIZED    1 << 2

/* Flags for thrd_pool_t::status */
#define THRD_POOL_ACTIVE    1 << 1 /**< Active pool (running threads) */
#define THRD_POOL_KILLALL   1 << 2 /**< All threads shutting down */
#define THRD_POOL_SORTED    1 << 3 /**< The pool threads are sorted */

#define THRD_ERR_LIBRARY    1 << 0 /**< Library error */
#define THRD_ERR_CLOSED     1 << 1 /**< Added thread on a running pool */
#define THRD_ERR_NULLPER    1 << 2 /**< Declared null period */

/* All error OR-ed, for cleanup, used by strerr */
#define THRD_ERR_ALL \
    ( THRD_ERR_LIBRARY | THRD_ERR_CLOSED | THRD_ERR_NULLPER )

/* Test for the condition variable of thrd_sync_t */
#define THRD_POOL_CONDITION \
    ( THRD_POOL_ACTIVE | THRD_POOL_KILLALL )


static
void * thread_routine (void * arg)
{
    thrd_t *thrd = (thrd_t *)arg;
    struct timespec next_act;

    DEBUG_TIMESPEC("Thread will start at:", thrd->info.delay);
    rtutils_wait(&thrd->info.delay);

    /* Periodic loop: at each cycle the next absoute activation time is
     * computed. */
    rtutils_get_now(&next_act);
    for (;;) {
        DEBUG_TIMESPEC("Execution!", next_act);
        rtutils_time_increment(&next_act, &thrd->info.period);

        //thrd->info.callback(thrd->info.context);
        rtutils_wait(&next_act);
    }

    pthread_exit(NULL);
}

static
int startup(thrd_t *thrd, struct timespec *enabtime)
{   
    pthread_attr_t attr;
    int err;
    struct sched_param param = {
        .sched_priority = thrd->priority
    };

    DEBUG_TIMESPEC("Activating thread with period", thrd->info.period);

    /* Make the activation wait absolute */
    rtutils_time_increment(&thrd->info.delay, enabtime);

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

    thrd->status |= THRD_ALIVE;
    return 0;
}

int thrd_add (thrd_pool_t *pool, thrd_info_t * new_thrd)
{
    thrd_t *item;

    if (pool->status & THRD_POOL_ACTIVE) {
        pool->status |= THRD_ERR_CLOSED;
        return -1;
    }

    assert(item = (thrd_t *) malloc(sizeof(thrd_t)));
    item->status = THRD_INITIALIZED;
    memcpy((void *)&item->info, (const void *)new_thrd, sizeof(thrd_info_t));
    pool->threads = dlist_append(pool->threads, (void *)item);

    return 0;
}

/* Comparsion between threads, allows to determine which has the smallest
 * period.
 */
static
int prio_cmp (const thrd_t *t0, const thrd_t *t1)
{
    return rtutils_time_cmp(&t0->info.period, &t1->info.period);
}

static
int set_rm_priorities (dlist_t **threads, int minprio)
{
    diter_t *i;
    dlist_t *ts;

    /* Sort by period */
    ts = *threads = dlist_sort(*threads, (dcmp_func_t) prio_cmp);
    i = dlist_iter_new(&ts);

    while (diter_hasnext(i)) {
        thrd_t *t;

        t = (thrd_t *) diter_next(i);
        if (rtutils_time_iszero(&t->info.period)) {
            /* Null period? Shame on you! */
            dlist_iter_free(i);
            return -1;
        }
        t->priority = minprio ++;
    }
    dlist_iter_free(i);
    return 0;
}

int thrd_start (thrd_pool_t *pool)
{
    diter_t *i;
    int err;
    struct timespec now;

    if ((pool->status & THRD_POOL_SORTED) == 0) {
        /* We need to bulid the priority set the first time! */
        if (set_rm_priorities(&pool->threads, pool->minprio) == -1) {
            pool->status |= THRD_ERR_NULLPER;
            return -1;
        }
    }

    pool->status |= THRD_POOL_ACTIVE;
    i = dlist_iter_new(&pool->threads);

    rtutils_get_now(&now);
    while (diter_hasnext(i)) {
        err = startup((thrd_t *) diter_next(i), &now);

        if (err != 0) {
            pool->status |= THRD_ERR_LIBRARY;
            pool->err = err;
            dlist_iter_free(i);

            return -1;
        }
    }
    dlist_iter_free(i);

    return 0;
}

const char * thrd_strerr (thrd_pool_t *pool)
{
    uint8_t status = pool->status;
    pool->status &= ~THRD_ERR_ALL;

    if (status & THRD_ERR_LIBRARY)
        return strerror(pool->err);
    if (status & THRD_ERR_CLOSED)
        return "Cannot add a thread on a running pool";
    if (status & THRD_ERR_NULLPER)
        return "Null period for RM thread";
    return "Unknown";
}

void thrd_init (thrd_pool_t *pool, int minprio)
{
    memset((void *) pool, 0, sizeof(thrd_pool_t));
    pool->minprio = minprio;
}

static
void free_thread (void *thrd)
{
    thrd_t *t;

    t = (thrd_t *)thrd;

    if (t->status & THRD_ALIVE) {
        pthread_join(t->handler, NULL);
    }
    free(t);
}

void thrd_destroy (thrd_pool_t *pool)
{
    dlist_free(pool->threads, free_thread);
}

