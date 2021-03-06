/*
 * Copyright 2010 Giovanni Simoni
 *
 * This file is part of Soto.
 *
 * Soto is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Soto is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Soto.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "headers/thrd.h"
#include "headers/rtutils.h"
#include "headers/config.h"

#include <sched.h>
#include <error.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>

/* Flags for thrd_t::status */
#define THRD_ALIVE          1 << 0
#define THRD_INITIALIZED    1 << 2

/* Flags for thrd_pool_t::status */
#define THRD_POOL_ACTIVE    1 << 7 /**< Active pool (running threads) */
#define THRD_POOL_KILLALL   1 << 6 /**< All threads shutting down */
#define THRD_POOL_SORTED    1 << 5 /**< The pool threads are sorted */

/* All error OR-ed, for cleanup, used by strerr */
#define THRD_ERR_ALL \
    ( THRD_ERR_LIBRARY | THRD_ERR_CLOSED | THRD_ERR_NULLPER | \
      THRD_ERR_EMPTY )

/* Test for the condition variable of thrd_sync_t */
#define THRD_POOL_CONDITION \
    ( THRD_POOL_ACTIVE | THRD_POOL_KILLALL )

struct thrd_pool {
    dlist_t *threads;        /**< List of thrd_t objects (@see thrd.c); */
    size_t nthreads;         /**< Number of sampling threads; */

    uint8_t status;          /**< Status flags: */
    int err;                 /**< Stores error codes; */
    int minprio;             /**< Minimum priority. */
};

/* Internal descriptor for a thread */
typedef struct {

    int priority;               /* Thread's priority; */
    pthread_t handler;          /* Handler of the thread; */
    uint8_t status;             /* Status flags; */
    thrd_info_t info;           /* User defined thread info. Defined in
                                   headers/thrd.h; */

    struct timespec start;      /* Pool start time, used for computing the
                                   delay during startup phase; */

    thrd_rtstats_t statistics;  /* Statistics about the task. */
} thrd_t; 

static
void update_statistics (thrd_rtstats_t *stats, uint64_t r, uint64_t f,
                        int deadline_miss)
{
    uint64_t response = f - r;

    stats->response_times += response;
    stats->n_executions ++;
    if (stats->wcrt < response) {
        stats->wcrt = response;
    }
    if (deadline_miss) {
        stats->dmiss_count ++;
        DEBUG_MSG("Deadline miss");
    }
}

/* Each real-time thread of this project actually corresponds to the
 * execution of this routine. */
static
void * thread_routine (void * arg)
{
    /* Thread management information is transparently keeped into the
     * thread stack. The external callback system gives the abstraction. */
    thrd_t *thrd = (thrd_t *)arg;
    struct timespec next_act;
    struct timespec finish_time;
    struct timespec arrival_time;
    void *context;

    context = thrd->info.context;

    /* If the user declared an initialization function we execute it. */
    if (thrd->info.init) {
        if (thrd->info.init(context)) {
            /* The user aborted thread startup. If there's a destructor
             * callback it's time to call it. */
            if (thrd->info.destroy) {
                thrd->info.destroy(context);
            }
            pthread_exit(NULL);
        }
    }

    /* Wait delayed activation. */
    rtutils_wait(&thrd->start);

    /* Periodic loop: at each cycle the next absoute activation time is
     * computed. */
    rtutils_get_now(&next_act);
    for (;;) {
        rtutils_time_copy(&arrival_time, &next_act);
        rtutils_time_increment(&next_act, &thrd->info.period);

        if (thrd->info.callback(context)) {
            /* Thread required to shut down. If there's a destructor
             * callback it shall be called now. */
            if (thrd->info.destroy) {
                thrd->info.destroy(context);
            }
            pthread_exit(NULL);
        }
        rtutils_get_now(&finish_time);

        update_statistics(&thrd->statistics,
                          rtutils_time2ns(&arrival_time),
                          rtutils_time2ns(&finish_time),
                          rtutils_time_cmp(&next_act, &finish_time) > 0);

        rtutils_wait(&next_act);
    }

    pthread_exit(NULL);
}

/* Thread startup */
static
int startup(thrd_t *thrd, struct timespec *enabtime)
{   
    #ifndef RT_DISABLE
    pthread_attr_t attr;
    struct sched_param param = {
        .sched_priority = thrd->priority
    };
    #endif

    int err;

    DEBUG_TIMESPEC("Activating thread with period", thrd->info.period);

    /* Build activation time for this call of start: we make a copy of the
     * enable time and increment it with thread specification delay.
     */
    memcpy((void *) &thrd->start, (const void *)enabtime,
           sizeof(struct timespec));
    rtutils_time_increment(&thrd->start, &thrd->info.delay);

    /* Setting stats to zero */
    memset(&thrd->statistics, 0, sizeof(thrd_rtstats_t));

    #ifndef RT_DISABLE
        /* Setting thread as real-time, scheduled as FIFO and with the given
         * priority. */
        err = pthread_attr_init(&attr);
        assert(err == 0);
        err = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        assert(err == 0);
        err = pthread_attr_setschedparam(&attr, &param);
        assert(err == 0);
        err = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        assert(err == 0);

        err = pthread_create(&thrd->handler, &attr, thread_routine,
                             (void *) thrd);
        pthread_attr_destroy(&attr);
    #else
        err = pthread_create(&thrd->handler, NULL, thread_routine,
                             (void *) thrd);
    #endif

    if (err != 0) return err;

    /* Activation achieved: update flags. */
    thrd->status |= THRD_ALIVE;
    return 0;
}

const thrd_rtstats_t * thrd_add (thrd_pool_t *pool,
                                 const thrd_info_t * new_thrd)
{
    thrd_t *item;

    /* There's no pending error */
    assert((pool->status & THRD_ERR_ALL) == 0);
    assert(new_thrd->callback);

    DEBUG_MSG("Added another thread");

    if (pool->status & THRD_POOL_ACTIVE) {
        pool->status |= THRD_ERR_CLOSED;
        return NULL;
    }

    item = (thrd_t *) malloc(sizeof(thrd_t));
    assert(item);
    item->status = THRD_INITIALIZED;
    memcpy((void *)&item->info, (const void *)new_thrd,
           sizeof(thrd_info_t));

    /* New thread is added to the thread list */
    pool->threads = dlist_append(pool->threads, (void *)item);

    return &item->statistics;
}

/* Comparsion between threads, allows to determine which has the smallest
 * period.
 */
static
int prio_cmp (const thrd_t *t0, const thrd_t *t1)
{
    return rtutils_time_cmp(&t0->info.period, &t1->info.period);
}

/* Sorts the list basing on the period, getting a rate-monotonic priority
 * assignment */
static
int set_rm_priorities (dlist_t **threads, int minprio)
{
    diter_t *i;
    dlist_t *ts;

    /* Sort by period */
    ts = *threads = dlist_sort(*threads, (dcmp_func_t) prio_cmp);
    i = dlist_iter_new(&ts);

    /* Assign priorities */
    while (diter_hasnext(i)) {
        thrd_t *t;

        t = (thrd_t *) diter_next(i);
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

    if (dlist_empty(pool->threads)) {
        pool->status |= THRD_ERR_EMPTY;
        return -1;
    }

    /* We need to bulid the priority set the first time.
     *
     * Currently this "first time" check is useless, since for the moment
     * this structure is not supporting thread stopping.
     */
    if ((pool->status & THRD_POOL_SORTED) == 0) {
        if (set_rm_priorities(&pool->threads, pool->minprio) == -1) {
            pool->status |= THRD_ERR_NULLPER;
            return -1;
        }
    }

    /* Start each thread. */
    pool->status |= THRD_POOL_ACTIVE;
    i = dlist_iter_new(&pool->threads);
    rtutils_get_now(&now);
    while (diter_hasnext(i)) {
        err = startup((thrd_t *) diter_next(i), &now);
        if (err) {
            pool->status |= THRD_ERR_LIBRARY;
            pool->err = err;
            dlist_iter_free(i);

            return -1;
        }
    }
    dlist_iter_free(i);

    return 0;
}

const char * thrd_strerr (thrd_pool_t *pool, thrd_err_t err)
{
    switch (err) {
        case THRD_ERR_LIBRARY:
            return strerror(pool->err);
        case THRD_ERR_CLOSED:
            return "Cannot add a thread on a running pool";
        case THRD_ERR_NULLPER:
            return "Null period for RM thread";
        case THRD_ERR_EMPTY:
            return "No threads subscribed";
    }
    return "Unknown";
}

thrd_err_t thrd_interr (thrd_pool_t *pool)
{
    uint8_t status = pool->status;
    pool->status &= ~THRD_ERR_ALL;  /* Reset pending errors */
    return status & THRD_ERR_ALL;
}

thrd_pool_t * thrd_new (unsigned minprio)
{
    thrd_pool_t *pool;

    pool = (thrd_pool_t *) calloc(1, sizeof(thrd_pool_t));
    assert(pool);
    pool->minprio = minprio + sched_get_priority_min(SCHED_FIFO);

    return pool;
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

