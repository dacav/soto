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
#include <thdacav/thdacav.h>
#include <stdint.h>
#include <pthread.h>

#include "headers/sampthread.h"
#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"

/* Sampling thread internal information set. */
struct sampth_data {
    snd_pcm_t *pcm;                     /* Alsa handler; */

    samp_frame_t * buffer;              /* Reading buffer; */
    snd_pcm_uframes_t slot_size;        /* Size of a sample; */
    size_t nslots;                    /* Room for samples; */
    unsigned slot;                      /* Slot cursor; */
    pthread_mutex_t mux;                /* Mutex protecting the cursor; */

    struct {
        int active;             /* 1 if the thread is running; */
        pthread_t self;         /* Handler used for termination; */
    } thread;

    /* Sometimes alsa has tantrums and issues EAGAIN on reading (despite
     * this is not documented anywere, LOL). In this case we wait up to
     * this value before giving up. */
    uint64_t alsa_wait_max;
};

/* This callback is pushed into the thread cleanup system. Acts like a
 * distructor, but works internally. This is better explained later. */
static
void destroy_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;

    pthread_mutex_destroy(&ctx->mux);

    free((void *)ctx->buffer);
    free(arg);
}

/* Thread initialization. */
static
int init_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    ctx->thread.active = 1;
    ctx->thread.self = pthread_self();
    return 0;
}

/* This function hides Alsa weird bogus under the hood. */
static
int alsa_read (snd_pcm_t *pcm, samp_frame_t *buffer,
               snd_pcm_uframes_t bufsize, int maxwait)
{
    int nread;

    nread = (int) snd_pcm_readi(pcm, buffer, bufsize);
    if (nread > 0) {
        /* Everything worked correctly. */
        return nread;
    }

    /* Note: alsa errors are negative */
    switch (nread) {
        case -EPIPE:
            LOG_MSG("Got overrun");
            if (snd_pcm_recover(pcm, nread, 0)) {
                LOG_MSG("Overrun handling failure");
            }
            nread = 1;
        case -EAGAIN:
            /* Smelly undocumented failure, which turns out to be
             * recoverable with a little waiting before trying again. This
             * can be achieved by snd_pcm_wait, which btw requires the
             * value expressed in microseconds (hence maxwait/10). */
            LOG_MSG("Waiting resource");
            nread = snd_pcm_wait(pcm, maxwait / 10);
    }

    /* After the recover we retry to read data once, if it fails again we
     * just skip to next activation and abort this job */
    switch (nread) {
        case 1:
            return (int) snd_pcm_readi(pcm, buffer, bufsize);
        case 0:
            return -EAGAIN;
        case -EPIPE:
            return snd_pcm_recover(pcm, nread, 0);
        default:
            /* Everything is badly documented here. Let the snd_strerr
             * decide what is this, I did the best effort to recover. */
            return nread;
    }
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
    nread = alsa_read(ctx->pcm, ctx->buffer + slot, ctx->slot_size,
                      ctx->alsa_wait_max);
    ctx->slot = (slot + 1) % ctx->nslots;
    pthread_mutex_unlock(&ctx->mux);

    if (nread <= 0) {
        LOG_FMT("Alsa fails: %s", snd_strerror(nread));
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

int sampth_subscribe (sampth_handler_t *handler, thrd_pool_t *pool,
                      const samp_t *samp, size_t scaling_factor)
{
    thrd_info_t thi;
    struct sampth_data *ctx;
    const struct timespec * period;
    int err;

    thi.init = init_cb;
    thi.callback = thread_cb;
    thi.destroy = NULL;

    /* Note: the thread is in charge of freeing this before shutting
     *       down, unless everything fails on thrd_add().
     */
    thi.context = calloc(1, sizeof(struct sampth_data));
    assert(thi.context);
    ctx = (struct sampth_data *) thi.context;
    ctx->pcm = samp_get_pcm(samp);
    ctx->thread.active = 0;

    ctx->nslots = scaling_factor;
    ctx->slot_size = samp_get_nframes(samp);
    ctx->buffer = (samp_frame_t *) calloc(scaling_factor * ctx->slot_size,
                                          sizeof(samp_frame_t));
    ctx->slot = 0;
    pthread_mutex_init(&ctx->mux, NULL);

    thi.delay.tv_sec = STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = STARTUP_DELAY_nSEC;

    /* Period management. */
    period = samp_get_period(samp);
    ctx->alsa_wait_max = rtutils_time2ns(period) / ALSA_WAIT_PROPORTION;
    rtutils_time_copy(&thi.period, period);     /* Period request to the thread pool */

    if ((err = thrd_add(pool, &thi)) != 0) {
        free(ctx->buffer);
        free(ctx);
        *handler = NULL;
    } else {
        *handler = ctx;
    }
    return err;
}

int sampth_sendkill (sampth_handler_t handler)
{
    if (handler == NULL) {
        return -1;
    }
    if (handler->thread.active) {
        int err;
        
        /* The cancellation will be checked explicitly in thread_cb().
         * Also note that the thread pool (see headers/thrd.h) is in
         * charge of joining the thread.
         */
        err = pthread_cancel(handler->thread.self);

        assert(!err);
        return 0;
    } else {
        return -1;
    }
}

