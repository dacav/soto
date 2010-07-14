#include "headers/thrd.h"

#include <sched.h>
#include <error.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define SECOND_NS 1000000000UL

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

static inline
void delay_activation (const struct timespec *delay)
{
    /* TODO add remain? */
    assert(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                           delay, NULL) == 0);
}

static
void * thread_routine (void * arg)
{
    thrd_t *thrd = (thrd_t *)arg;

    DEBUG_FMT("Thread ready, starting at {%u,%lu}\n",
              (unsigned) thrd->info.delay.tv_sec,
              thrd->info.delay.tv_nsec);
    delay_activation(&thrd->info.delay);

    pthread_exit(NULL);
}

/* Make absolute a certain time delay by adding some 'current time' */
static inline
void make_absolute (struct timespec *delay, const struct timespec *now)
{
    long nano;

    delay->tv_sec += now->tv_sec;
    nano = now->tv_nsec + delay->tv_nsec;
    if (nano > SECOND_NS) {
        delay->tv_sec ++;
        nano -= SECOND_NS;
    }
    delay->tv_nsec = nano;
}

static
int startup(thrd_t *thrd, struct timespec *enabtime)
{   
    pthread_attr_t attr;
    int err;
    struct sched_param param = {
        .sched_priority = thrd->info.priority
    };

    make_absolute(&thrd->info.delay, enabtime);

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

/* Comparsion between timespec structures, allows to determine which
 * period is greater
 */
static
int timespec_cmp (const struct timespec *s0, const struct timespec *s1)
{
    if (s0->tv_sec == s1->tv_sec)
        return s0->tv_nsec > s1->tv_nsec ? -1 : 1;
    return s0->tv_sec > s1->tv_sec ? -1 : 1;
}

static inline
int timespec_iszero (const struct timespec *s)
{
    return s->tv_sec == 0 && s->tv_nsec == 0;
}

/* Comparsion between threads, allows to determine which has the greatest
 * priority.
 */
static
int prio_cmp (const thrd_t *t0, const thrd_t *t1)
{
    int types = 0;

    if (t0->info.prio_type == THRD_PRIO_EXPL) {
        types |= 1 << 0;
    }
    if (t0->info.prio_type == THRD_PRIO_EXPL) {
        types |= 1 << 1;
    }

    switch (types) {
        case 0:     // both THRD_PRIO_RM
            return timespec_cmp(&t0->info.period,
                                &t1->info.period);
        case 1:     // only t0 THRD_PRIO_EXPL
            return -1;
        case 2:     // only t1 THRD_PRIO_EXPL
            return 1;
        case 3:     // both THRD_PRIO_EXPL
            return t0->info.priority - t1->info.priority;
    }
    abort();        // impossible things sometimes happens.
}

static
int set_rm_priorities (dlist_t **threads, int minprio)
{
    diter_t *i;
    dlist_t *ts = *threads;

    /* First sorting: RM threads ordered by period, while explicit
     * priority threads are considered with lower priority; */
    ts = dlist_sort(ts, (dcmp_func_t) prio_cmp);

    /* Now we can compute priority basing on period for RM threads, and
     * transform them into explicit-priority threads; */
    i = dlist_iter_new(&ts);

    while (diter_hasnext(i)) {
        thrd_t *t;

        t = (thrd_t *) diter_next(i);

        if (t->info.prio_type == THRD_PRIO_EXPL) {
            break;
        }
        if (timespec_iszero(&t->info.period)) {
            /* Null period? Shame on you! */
            dlist_iter_free(i);
            return -1;
        }
        t->info.prio_type = THRD_PRIO_EXPL;
        t->info.priority = minprio ++;
    }
    dlist_iter_free(i);

    /* Second sorting will build the correct priority assignment. */
    *threads = dlist_sort(ts, (dcmp_func_t) prio_cmp);

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

    clock_gettime(CLOCK_MONOTONIC, &now);
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

