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

#include <thdacav/thdacav.h>
#include <stdint.h>

#include "headers/show.h"
#include "headers/logging.h"
#include "headers/alsagw.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/sampthread.h"

struct showth_data {
    samp_frame_t *buffer;
    snd_pcm_uframes_t buflen;
    sampth_t *sampth;

    plotgr_t *g0, *g1;

    struct {
        int active;             /* 1 if the thread is running; */
        pthread_t self;         /* Handler used for termination; */
    } thread;
};

static
int init_cb (void *arg)
{
    struct showth_data *ctx = (struct showth_data *)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    ctx->thread.active = 1;
    ctx->thread.self = pthread_self();
    return 0;
}

static
void destroy_cb (void *arg)
{
    struct showth_data *ctx = (struct showth_data *)arg;

    free(ctx->buffer);
    free(arg);
}

static
int thread_cb (void *arg)
{
    unsigned i;
    struct showth_data *ctx = (struct showth_data *)arg;
    samp_frame_t *buffer;

    buffer = ctx->buffer;
    sampth_get_samples(ctx->sampth, buffer);
    for (i = 0; i < ctx->buflen; i ++) {
        plot_graphic_set(ctx->g0, i, buffer[i].ch0);
        plot_graphic_set(ctx->g1, i, buffer[i].ch1);
    }

    /* Check cancellations. If it is the case the destroy_cb will manage
     * memory deallocation. */
    pthread_cleanup_push(destroy_cb, arg);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_cleanup_pop(0);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    return 0;
}

int showth_subscribe (showth_t **handle, thrd_pool_t *pool,
                      sampth_t *sampth, plotgr_t *g0, plotgr_t *g1)
{
    thrd_info_t thi;
    struct showth_data *ctx;
    int err;

    thi.init = init_cb;
    thi.callback = thread_cb;
    thi.destroy = NULL;

    ctx = calloc(1, sizeof(struct showth_data));
    assert(ctx);
    thi.context = (void *) ctx;

    /* Common startup delay. */
    thi.delay.tv_sec = SAMP_STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = SAMP_STARTUP_DELAY_nSEC;

    /* The startup delay must be incremented in order to allow the
     * sampling thread to fill at least one buffer. The same value is used
     * to set the period. */
    rtutils_time_increment(&thi.delay, sampth_get_period(sampth));
    rtutils_time_copy(&thi.period, &thi.delay);

    ctx->buflen = sampth_get_size(sampth);
    ctx->buffer = calloc(ctx->buflen, sizeof(samp_frame_t));
    assert(ctx->buffer);

    ctx->thread.active = 0;
    ctx->sampth = sampth;
    ctx->g0 = g0;
    ctx->g1 = g1;

    if ((err = thrd_add(pool, &thi)) != 0) {
        free(ctx->buffer);
        free(ctx);
        *handle = NULL;
    } else {
        *handle = ctx;
    }
    return err;
}

int showth_sendkill (showth_t *handle)
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

