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

#include "headers/sampthread.h"
#include "headers/logging.h"
#include "headers/rtutils.h"

struct context {
    snd_pcm_t *pcm;
    samp_frame_t * buffer;
    snd_pcm_uframes_t bufsize;  // in frames.
    thdqueue_t *output;
};

static
samp_frame_t * samp_buffer_dup (const samp_frame_t *buf,
                                snd_pcm_uframes_t size)
{
    void *ret;

    ret = malloc(size * sizeof(samp_frame_t));
    assert(ret);
    memcpy(ret, (const void *)buf, size * sizeof(samp_frame_t));

    return ret;
}

static
int thread_cb (void *arg)
{
    struct context *ctx = (struct context *) arg;
    snd_pcm_uframes_t bufsize, nread;

    bufsize = ctx->bufsize;
    nread = snd_pcm_readi(ctx->pcm, ctx->buffer, ctx->bufsize);
    if (nread < 0) {
        LOG_FMT("Failure in alsa reading: %s", snd_strerror(nread));
    } else { 
        if (nread < bufsize) {
            LOG_FMT("Overrun detected: read %d of %d", (int) nread, (int) bufsize);
        }
        if (thdqueue_insert(ctx->output, samp_buffer_dup(ctx->buffer,
                bufsize)) == THDQUEUE_UNALLOWED) {
            return 1;   // Queue dropped! Termination.
        }
    }
    return 0;
}

static
int destroy_cb (void *arg)
{
    struct context *ctx = (struct context *) arg;

    free((void *)ctx->buffer);
    free(arg);

    return 0;
}

int sampthread_create (thrd_pool_t *pool, const samp_t *samp,
                       thdqueue_t *output)
{
    thrd_info_t thi;
    struct context *ctx;

    thi.init = NULL;
    thi.callback = thread_cb;
    thi.destroy = destroy_cb;

    /* Note: the thread is in charge of freeing this before shutting
     * down. */
    thi.context = malloc(sizeof(struct context));
    assert(thi.context);
    ctx = (struct context *) thi.context;
    ctx->output = output;
    ctx->pcm = samp_get_pcm(samp);
    ctx->bufsize = samp_get_nframes(samp);
    ctx->buffer = (samp_frame_t *) malloc(ctx->bufsize * sizeof(samp_frame_t));

    thi.delay.tv_sec = STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = STARTUP_DELAY_nSEC;

    memcpy((void *) &thi.period, (const void *) samp_get_period(samp),
            sizeof(struct timespec));

    return thrd_add(pool, &thi);
}

