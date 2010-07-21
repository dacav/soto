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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>

#include <dacav/dacav.h>
#include <thdacav/thdacav.h>

#include <alsa/asoundlib.h>

#include "headers/options.h"
#include "headers/alsagw.h"
#include "headers/thrd.h"
#include "headers/sampthread.h"
#include "headers/rtutils.h"

static
int init_samp_by_opts (samp_t *samp, opts_t *opts)
{
    samp_info_t info;

    info.device = opts->device;
    info.rate = opts->rate;
    info.nsamp = opts->nsamp;
    info.channels = opts->mode == MONO ? 1 : 2;

    return samp_init(samp, &info, opts->policy);
}

#if 0
static
void dump_buffer (samp_frame_t *buf, snd_pcm_uframes_t n)
{
    samp_frame_t *cursor = buf;

    assert(n);
    while (n--) {
        LOG_FMT("left=%i right=%i", cursor->ch0, cursor->ch1);
        cursor ++;
    }
    DEBUG_FMT("Buffer [%p] completed... next", (void *) buf);
}
#endif

static
void read_data (thdqueue_t *q, snd_pcm_uframes_t nframes)
{
    for (;;) {
        samp_frame_t *buf;

        if (thdqueue_extract(q, (void **) &buf) == THDQUEUE_ENDDATA) {
            return;
        }
        //DEBUG_FMT("EXTRACT: buflen=%d", (int) nframes);
        //dump_buffer(buf, nframes);
        free (buf);
    }
}

struct read_data {
    thdqueue_t *queue;
    snd_pcm_uframes_t nframes;
};

static
int reading_cb (void *arg)
{
    struct read_data *data = (struct read_data *) arg;
    DEBUG_MSG("STARTING TO READ");
    read_data(data->queue, data->nframes);
    return 1;
}

static
int create_reading_thread (thrd_pool_t *pool, struct read_data *rd)
{
    thrd_info_t th = {
        .init = NULL,
        .callback = reading_cb,
        .destroy = NULL,
        .context = (void *) rd,
        .period = {
            .tv_sec = 1,
            .tv_nsec = 0
         },
        .delay = {
            .tv_sec = 0,
            .tv_nsec = 0
         }
    };
    return thrd_add(pool, &th);
}

int main (int argc, char **argv)
{
    opts_t opts;
    samp_t sampler;
    thrd_pool_t pool;
    struct read_data rd;
    int err;
    sampth_handler_t h;

    DEBUG_MSG("STEP!");

    if (opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }
    thrd_init(&pool, opts.minprio);

    if (init_samp_by_opts(&sampler, &opts)) {
        samp_err_t err = samp_interr(&sampler);

        ERR_FMT("Sampler init: %s", samp_strerr(&sampler, err));
        exit(EXIT_FAILURE);
    }

    /* Populate the thread pool */
    rd.queue = thdqueue_new();
    if (sampth_subscribe(&h, &pool, &sampler, rd.queue)) {
        thrd_err_t err = thrd_interr(&pool);

        ERR_FMT("Pool init: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    rd.nframes = samp_get_nframes(&sampler);
    if ((err = create_reading_thread(&pool, &rd)) != 0) {
        thrd_err_t err = thrd_interr(&pool); 

        ERR_FMT("Adding reader: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    if (thrd_start(&pool)) {
        thrd_err_t err = thrd_interr(&pool);

        ERR_FMT("Starting init: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    DEBUG_MSG("KILLING SPREAD");
    sleep(2);
    DEBUG_MSG("Sending my bill");
    if (sampth_sendkill(h)) {
        DEBUG_MSG("...but I failed miserably");
    } else {
        DEBUG_MSG("...And I succeded in it");
    }

    thrd_destroy(&pool);
    samp_destroy(&sampler);

    exit(EXIT_SUCCESS);
}
