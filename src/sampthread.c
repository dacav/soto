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

#include "headers/sampthread.h"
#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"

struct sampth_data {
    snd_pcm_t *pcm;
    samp_frame_t * buffer;
    snd_pcm_uframes_t bufsize;  // in frames.
    thdqueue_t *output;

    struct {
        int active;
        pthread_t self;
    } thread;
};

#define EASY

static
sampth_frameset_t * build_frameset (snd_pcm_uframes_t size,
                                    const samp_frame_t *buf)
{
    sampth_frameset_t *ret;
#ifndef EASY
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
#ifndef EASY
    free(set);
#else
    free(set->frames);
    free(set);
#endif
}

sampth_frameset_t * sampth_frameset_dup (const sampth_frameset_t *set)
{
    return build_frameset(set->nframes, set->frames);
}

static
void destroy_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;

    thdqueue_enddata(ctx->output);

    free((void *)ctx->buffer);
    free(arg);
}

static
int init_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    ctx->thread.active = 1;
    ctx->thread.self = pthread_self();
    return 0;
}

static
int thread_cb (void *arg)
{
    struct sampth_data *ctx = (struct sampth_data *) arg;
    unsigned bufsize;
    int nread;

    bufsize = (unsigned) ctx->bufsize;
    nread = (int) snd_pcm_readi(ctx->pcm, ctx->buffer, ctx->bufsize);
    if (nread == -EPIPE) {
        LOG_MSG("Got Overrun");
        if (snd_pcm_recover(ctx->pcm, -EPIPE, 0)) {
            LOG_MSG("Overrun not handled successfully :(");
        }
    } else if (nread < 0) {
        LOG_FMT("Alsa fails miserably: %s", snd_strerror(nread));
    } else { 
        sampth_frameset_t *topush;

        topush = build_frameset(nread, ctx->buffer);
        if (thdqueue_insert(ctx->output, (void *)topush)
                == THDQUEUE_UNALLOWED) {
            sampth_frameset_destroy(topush);
            return 1;   // Queue dropped! Termination.
        }
    }

    // Check cancellations.
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

    memcpy((void *) &thi.period, (const void *) samp_get_period(samp),
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
        
        err = pthread_cancel(handler->thread.self);
        assert(!err);
        return 0;
    } else {
        return -1;
    }
}

