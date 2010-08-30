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

#include "headers/signal_show.h"
#include "headers/logging.h"
#include "headers/alsagw.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/sampthread.h"

struct signth_data {
    alsagw_frame_t *buffer;
    snd_pcm_uframes_t buflen;
    genth_t *sampth;

    plotgr_t *g0, *g1;
};

static
int destroy_cb (void *arg)
{
    struct signth_data *ctx = (struct signth_data *)arg;

    free(ctx->buffer);
    free(arg);

    return 0;
}

static
int thread_cb (void *arg)
{
    unsigned i;
    struct signth_data *ctx = (struct signth_data *)arg;
    alsagw_frame_t *buffer;

    buffer = ctx->buffer;
    sampth_get_samples(ctx->sampth, buffer);
    for (i = 0; i < ctx->buflen; i ++) {
        plot_graphic_set(ctx->g0, i, buffer[i].ch0);
        plot_graphic_set(ctx->g1, i, buffer[i].ch1);
    }

    return 0;
}

const thrd_rtstats_t * signth_subscribe (genth_t **handle,
                                         thrd_pool_t *pool,
                                         genth_t *sampth,
                                         plotgr_t *g0,
                                         plotgr_t *g1)
{
    struct signth_data *ctx;
    thrd_info_t thi;
    const thrd_rtstats_t * err;

    thi.init = NULL;
    thi.callback = thread_cb;
    thi.destroy = destroy_cb;

    ctx = (struct signth_data *) calloc(1, sizeof(struct signth_data));
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
    ctx->buffer = calloc(ctx->buflen, sizeof(alsagw_frame_t));
    assert(ctx->buffer);

    ctx->g0 = g0;
    ctx->g1 = g1;
    ctx->sampth = sampth;

    if ((err = genth_subscribe(handle, pool, &thi)) == NULL) {
        free(ctx->buffer);
        free(ctx);
    }
    return err;
}

