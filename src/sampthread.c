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

#include <signal.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <pthread.h>

#include "headers/sampthread.h"
#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"

/* Sampling thread internal information set. */
struct sampth_data {
    alsagw_t *sampler;                    /* Alsa handler; */

    alsagw_frame_t * buffer;              /* Reading buffer; */
    snd_pcm_uframes_t slot_size;        /* Size of a sample; */
    size_t nslots;                      /* Room for samples; */
    unsigned slot;                      /* Slot cursor; */
    pthread_mutex_t mux;                /* Mutex protecting the cursor; */

    /* Sometimes alsa has tantrums and issues EAGAIN on reading (despite
     * this is not documented anywere, LOL). In this case we wait up to
     * this value before giving up. */
    uint64_t alsa_wait_max;

    /* Total period required to fill in the whole buffer, namely the
     * execution period multiplied by the number of slots */
    struct timespec read_period;
};

/* This callback is pushed into the thread cleanup system. Acts like a
 * distructor, but works internally. This is better explained later. */
static
int destroy_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;

    pthread_mutex_destroy(&ctx->mux);
    free((void *)ctx->buffer);

    return 0;
}

/* Core of the sampling */
static
int thread_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;
    unsigned slot;
    int nread;

    pthread_mutex_lock(&ctx->mux);
    slot = ctx->slot;
    nread = alsagw_read(ctx->sampler, ctx->buffer + slot * ctx->slot_size,
                      ctx->slot_size, ctx->alsa_wait_max);
    ctx->slot = (slot + 1) % ctx->nslots;
    pthread_mutex_unlock(&ctx->mux);

    if (nread <= 0) {
        LOG_FMT("Alsa fails: %s", snd_strerror(nread));
    }

    return 0;
}

const thrd_rtstats_t * sampth_subscribe (genth_t **handler,
                                         thrd_pool_t *pool,
                                         alsagw_t *samp,
                                         size_t scaling_factor)
{
    thrd_info_t thi;
    struct sampth_data *ctx;
    const struct timespec * period;
    const thrd_rtstats_t * err;

    thi.init = NULL;
    thi.callback = thread_cb;
    thi.destroy = destroy_cb;

    /* Note: the thread is in charge of freeing this before shutting
     *       down, unless everything fails on genth_subscribe().
     */
    thi.context = calloc(1, sizeof(struct sampth_data));
    assert(thi.context);
    ctx = (struct sampth_data *) thi.context;
    ctx->sampler = samp;

    DEBUG_FMT("Scaling factor %d", (int) scaling_factor);
    ctx->nslots = scaling_factor;
    ctx->slot_size = alsagw_get_nframes(samp);
    ctx->buffer = (alsagw_frame_t *) calloc(scaling_factor * ctx->slot_size,
                                          sizeof(alsagw_frame_t));
    ctx->slot = 0;
    pthread_mutex_init(&ctx->mux, NULL);

    thi.delay.tv_sec = SAMP_STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = SAMP_STARTUP_DELAY_nSEC;

    /* Period management. */
    period = alsagw_get_period(samp);
    rtutils_time_copy(&ctx->read_period, period);
    rtutils_time_multiply(&ctx->read_period, scaling_factor);

    /* How much should I be locked waiting for a not-available data? */
    ctx->alsa_wait_max = ALSA_WAIT_PROPORTION != 0 ?
                         rtutils_time2ns(period) / ALSA_WAIT_PROPORTION :
                         0;

    /* Period request for the thread pool */
    rtutils_time_copy(&thi.period, period);

    if ((err = genth_subscribe(handler, pool, &thi)) == 0) {
        free(ctx->buffer);
        free(ctx);
    }
    return err;
}

snd_pcm_uframes_t sampth_get_size (const genth_t *handler)
{
    struct sampth_data *ctx = genth_get_context(handler);
    return ctx->slot_size * ctx->nslots;
}

void sampth_get_samples (genth_t *handler, alsagw_frame_t buffer[])
{
    struct sampth_data *ctx = genth_get_context(handler);
    const size_t sls = ctx->slot_size;
    unsigned slot;
    snd_pcm_uframes_t nframes;

    pthread_mutex_lock(&ctx->mux);
    slot = ctx->slot;
    nframes = sls * (ctx->nslots - slot);
    memcpy((void *)buffer,
           (const void *)&ctx->buffer[sls * slot],
           sizeof(alsagw_frame_t) * nframes);
    memcpy((void *)(buffer + nframes),
           (const void *)ctx->buffer,
           sizeof(alsagw_frame_t) * sls * slot);
    pthread_mutex_unlock(&ctx->mux);
}

const struct timespec * sampth_get_period (const genth_t *handler)
{
    struct sampth_data *ctx = genth_get_context(handler);
    return &ctx->read_period;
}

