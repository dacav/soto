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

#include "headers/sampthread.h"
#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"

/* Sampling thread internal information set. */
struct sampth_data {
    snd_pcm_t *pcm;             /* Alsa handler; */
    samp_frame_t * buffer;      /* Reading buffer; */
    snd_pcm_uframes_t bufsize;  /* And its size; */
    thdqueue_t *output;         /* Output queue for sampled data; */

    struct {
        int active;             /* 1 if the thread is running; */
        pthread_t self;         /* Handler used for termination; */
    } thread;

    /* Sometimes alsa has tantrums and issues EAGAIN on reading (despite
     * this is not documented anywere, LOL). In this case we wait up to
     * this value before giving up */
    uint64_t alsa_wait_max;
};

/* This define removes the optimization for the following two functions:
 * the optimimization i's not working for the moment. It's easy to fix it
 * but not a development priority. */
#define UNEFFICIENT_BUT_WORKING

static
sampth_frameset_t * build_frameset (snd_pcm_uframes_t size,
                                    const samp_frame_t *buf)
{
    sampth_frameset_t *ret;
#ifndef UNEFFICIENT_BUT_WORKING
    // FIXME this is buggy
    uint8_t *pos;

    /* All-in-one to reduce the number of malloc. This is not very clean,
     * but I guess it's better in a real-time situation.
     */
    assert(size > 0);
    ret = malloc(sizeof(sampth_frameset_t) + size * sizeof(samp_frame_t));
    assert(ret);

    pos = (uint8_t *) ret;
    pos += sizeof(samp_frame_t);
    ((sampth_frameset_t *)ret)->nframes = size;
    ((sampth_frameset_t *)ret)->frames = (samp_frame_t *)pos;

    memcpy((void *)pos, (const void *)buf, size * sizeof(samp_frame_t));
#else
    ret = malloc(sizeof(sampth_frameset_t));
    ret->nframes = size;
    ret->frames = malloc(sizeof(samp_frame_t) * size);
#endif

    return ret;
}

void sampth_frameset_destroy (sampth_frameset_t *set)
{
#ifdef UNEFFICIENT_BUT_WORKING
    free(set->frames);
#endif
    free(set);
}

sampth_frameset_t * sampth_frameset_dup (const sampth_frameset_t *set)
{
    return build_frameset(set->nframes, set->frames);
}

/* This callback is pushed into the thread cleanup system. Acts like a
 * distructor, but works internally. This is better explained later. */
static
void destroy_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;

    thdqueue_enddata(ctx->output);

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

/* This function hides Alsa under weird stuff the hood. */
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
    unsigned bufsize;
    int nread;

    bufsize = (unsigned) ctx->bufsize;

    nread = alsa_read(ctx->pcm, ctx->buffer, ctx->bufsize,
                      ctx->alsa_wait_max);
    if (nread <= 0) {
        LOG_FMT("Alsa fails: %s", snd_strerror(nread));
    } else { 
        /* We were able to read some useful data, which is pushed into the
         * output queue. */
        sampth_frameset_t *topush;

        topush = build_frameset(nread, ctx->buffer);
        if (thdqueue_insert(ctx->output, (void *)topush)
                == THDQUEUE_UNALLOWED) {
            /* This happens because someone required the data termination
             * through the queue. Actually termination works differently,
             * but here I don't assume anything about who is manipulating
             * the queue externally. */

            /* This element has not been pushed, so let's destroy it before
             * exiting. */
            sampth_frameset_destroy(topush);

            /* By returning 1 I ask the thread to be terminated. */
            return 1;
        }
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
                      const samp_t *samp, thdqueue_t *output)
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
    thi.context = malloc(sizeof(struct sampth_data));
    assert(thi.context);
    ctx = (struct sampth_data *) thi.context;
    ctx->output = output;
    ctx->pcm = samp_get_pcm(samp);
    ctx->bufsize = samp_get_nframes(samp);
    ctx->buffer = (samp_frame_t *) malloc(ctx->bufsize * sizeof(samp_frame_t));
    ctx->thread.active = 0;

    thi.delay.tv_sec = STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = STARTUP_DELAY_nSEC;

    /* Period management. */
    period = samp_get_period(samp);
    ctx->alsa_wait_max = rtutils_time2ns(period) / ALSA_WAIT_PROPORTION;

    /* Period request to the thread pool */
    memcpy((void *) &thi.period, (const void *) period,
            sizeof(struct timespec));

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

