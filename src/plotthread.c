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

#include <stdint.h>

#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/plotthread.h"
#include "headers/thrd.h"

struct plotth_data {
    plot_t *plot;

    struct {
        int active;             /* 1 if the thread is running; */
        pthread_t self;         /* Handler used for termination; */
    } thread;
};

static
int init_cb (void *arg)
{
    struct plotth_data *ctx = (struct plotth_data *)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    ctx->thread.active = 1;
    ctx->thread.self = pthread_self();
    return 0;
}

static
void destroy_cb (void *arg)
{
    free(arg);
}

static
int thread_cb (void *arg)
{
    struct plotth_data *ctx = (struct plotth_data *)arg;

    plot_redraw(ctx->plot);

    /* Check cancellations. If it is the case the destroy_cb will manage
     * memory deallocation. */
    pthread_cleanup_push(destroy_cb, arg);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_cleanup_pop(0);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    return 0;
}

int plotth_subscribe (plotth_t **handle, thrd_pool_t *pool,
                      plot_t *plot)
{
    thrd_info_t thi;
    struct plotth_data *ctx;
    int err;

    thi.init = init_cb;
    thi.callback = thread_cb;
    thi.destroy = NULL;

    ctx = calloc(1, sizeof(struct plotth_data));
    assert(ctx);
    thi.context = (void *) ctx;

    /* Plotting thread starts immediatly */
    thi.delay.tv_sec = 0;
    thi.delay.tv_nsec = 0;

    thi.period.tv_sec = PLOT_PERIOD_SEC;
    thi.period.tv_nsec = PLOT_PERIOD_nSEC;

    ctx->thread.active = 0;
    ctx->plot = plot;

    if ((err = thrd_add(pool, &thi)) != 0) {
        free(ctx);
        *handle = NULL;
    } else {
        *handle = ctx;
    }
    return err;
}

int plotth_sendkill (plotth_t *handle)
{
    if (handle == NULL) {
        return -1;
    }
    if (handle->thread.active) {
        int err;
        
        /* The cancellation will be checked explicitly in thread_cb().
         * Also note that the thread pool (see headers/thrd.h) is in
         * charge of joining the thread.
         */
        err = pthread_cancel(handle->thread.self);

        assert(!err);
        return 0;
    } else {
        return -1;
    }
}

