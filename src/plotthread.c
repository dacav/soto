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

#include "headers/plotthread.h"
#include "headers/logging.h"
#include "headers/alsagw.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/sampthread.h"

static
struct timespec build_period (const samp_info_t *spec)
{
    uint64_t sample_time = SECOND_NS / spec->rate;
    return rtutils_ns2time(PLOT_PERIOD_TIMES * sample_time *
                           spec->nsamp);
}

struct plotth_data {
    thdqueue_t * input;
    plot_t plot;
    unsigned rate;

    size_t nread;       /* What is left of the previous iteration */
    int32_t accum_c0;   /* Accumulator for channel 0 */
    int32_t accum_c1;   /* Accumulator for channel 1 */
};

static
int init_cb (void *arg)
{
    struct plotth_data *ctx = (struct plotth_data *) arg;
    plot_init(&ctx->plot, ctx->rate);

    return 0;
}

static
void update_plot (struct plotth_data *ctx, sampth_frameset_t *set)
{
    size_t nframes;
    int i, j = 0;

    nframes = set->nframes;
    while (nframes + ctx->nread >= PLOT_AVERAGE_LEN) {
        samp_frame_t frame;

        for (i = ctx->nread; i < PLOT_AVERAGE_LEN; i ++) {
            ctx->accum_c0 = set->frames[j].ch0;
            ctx->accum_c1 = set->frames[j].ch1;

            j ++;
        }
        frame.ch0 = ctx->accum_c0 / PLOT_AVERAGE_LEN;
        frame.ch1 = ctx->accum_c1 / PLOT_AVERAGE_LEN;
        plot_add_frame(&ctx->plot, &frame);
        nframes -= PLOT_AVERAGE_LEN;
        nframes += ctx->nread;
        ctx->accum_c0 = ctx->accum_c1 = ctx->nread = 0;
    }
    for (i = 0; i < nframes; i ++) {
        ctx->accum_c0 = set->frames[j].ch0;
        ctx->accum_c1 = set->frames[j].ch1;
        j ++;
    }
    ctx->nread = nframes;
}

static
int thread_cb (void *arg)
{
    struct plotth_data *ctx = (struct plotth_data *) arg;
    sampth_frameset_t *set;

    int k = 0;

    for (;;) {
        switch (thdqueue_try_extract(ctx->input, (void **) &set)) {
            case THDQUEUE_EMPTY:
                /* Nothing left to plot */
                return 0;
            case THDQUEUE_ENDDATA:
                /* Producer interrupted data stream */
                return 1;
            case THDQUEUE_SUCCESS:
                /* Updating new set */
                update_plot(ctx, set);
                k ++;
                sampth_frameset_destroy(set);
                break;
            default:
                DEBUG_MSG("Bug in libthdacav?");
                abort();
        }
    }

    return 0;
}

static
int destroy_cb (void *arg)
{
    struct plotth_data *ctx = (struct plotth_data *) arg;

    plot_destroy(&ctx->plot);
    free(arg);

    return 0;
}

int plotth_subscribe (thrd_pool_t *pool, thdqueue_t *input,
                      const samp_info_t *samp)
{
    thrd_info_t thi;
    struct plotth_data *ctx;
    struct timespec period;
    int err;

    thi.init = init_cb;
    thi.callback = thread_cb;
    thi.destroy = destroy_cb;

    /* Note: the thread is in charge of freeing this before shutting
     *       down, unless everything fails on thrd_add().
     */
    thi.context = malloc(sizeof(struct plotth_data));
    assert(thi.context);
    ctx = (struct plotth_data *) thi.context;
    ctx->input = input;
    ctx->rate = samp->rate;
    ctx->nread = 0;

    thi.delay.tv_sec = STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = STARTUP_DELAY_nSEC;

    period = build_period(samp),
    memcpy((void *) &thi.period, (const void *) &period,
            sizeof(struct timespec));

    if ((err = thrd_add(pool, &thi)) != 0) {
        free(ctx);
    }
    return err;
}


