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
#include "headers/alsasample.h"
#include "headers/thrd.h"
#include "headers/sampthread.h"

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

static
void dump_buffer (samp_frame_t *buf, snd_pcm_uframes_t n)
{
    samp_frame_t *cursor = buf;

    while (n--) {
        fprintf(stdout, "left=%i right=%i\n", cursor->ch0, cursor->ch1);
        cursor ++;
    }
    DEBUG_FMT("Buffer [%p] completed... next", (void *) buf);
}

static
void read_data (thdqueue_t *q, snd_pcm_uframes_t nframes)
{
    for (;;) {
        samp_frame_t *buf;

        if (thdqueue_extract(q, (void **) &buf) == THDQUEUE_ENDDATA) {
            return;
        }
        dump_buffer (buf, nframes);
        free (buf);
    }
}

int main (int argc, char ** argv)
{
    opts_t opts;
    samp_t sampler;
    thrd_pool_t pool;
    thdqueue_t *queue;

    if (opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }
    thrd_init(&pool, opts.minprio);

    if (init_samp_by_opts(&sampler, &opts)) {
        samp_err_t err = samp_interr(&sampler);

        fprintf(stderr, "Sampler init: %s\n", samp_strerr(&sampler, err));
        exit(EXIT_FAILURE);
    }

    /* Populate the thread pool */
    queue = thdqueue_new();
    sampthread_create(&pool, &sampler, queue);
    if (thrd_start(&pool)) {
        thrd_err_t err = thrd_interr(&pool);

        fprintf(stderr, "Pool init: %s\n", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    read_data(queue, samp_get_nframes(&sampler));

    thrd_destroy(&pool);
    samp_destroy(&sampler);

    exit(EXIT_SUCCESS);
}
