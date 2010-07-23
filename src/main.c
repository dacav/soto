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
#include "headers/plotting.h"
#include "headers/dispatch.h"

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
void update_plot (plot_t *l, plot_t *r, samp_frame_t *frames,
                  snd_pcm_uframes_t n)
{
    int i;

    for (i = 0; i < n; i ++) {
        plot_add_value(l, frames[i].ch0);
        plot_add_value(r, frames[i].ch1);
    }
}

static
void plot_data (thdqueue_t *q)
{
    plot_t left, right;
    sampth_frameset_t *bunch;

    plot_init(&left, 1000);
    plot_init(&right, 1000);
    while (thdqueue_extract(q, (void **) &bunch) != THDQUEUE_ENDDATA) {
        update_plot(&left, &right, bunch->frames, bunch->nframes);
        sampth_frameset_destroy(bunch);
    }
}

static
int plotting_cb (void *arg)
{
    DEBUG_MSG("STARTING TO PLOT");
    plot_data((thdqueue_t *)arg);
    abort();    // FIXME you should be able to exit if there's nothing
                // enqueued!!
    return 0;
}

static
void read_data (thdqueue_t *q)
{
    sampth_frameset_t *bunch;

    while (thdqueue_extract(q, (void **) &bunch) != THDQUEUE_ENDDATA) {
        sampth_frameset_destroy(bunch);
    }
}

static
int reading_cb (void *arg)
{
    DEBUG_MSG("STARTING TO READ");
    read_data((thdqueue_t *)arg);
    abort();    // FIXME same stuff
    return 0;
}

static
int create_plotting_thread (thrd_pool_t *pool, thdqueue_t *q)
{
    assert(q != NULL);

    thrd_info_t th = {
        .init = NULL,
        .callback = plotting_cb,
        .destroy = NULL,
        .context = (void *) q,
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


static
int create_reading_thread (thrd_pool_t *pool, thdqueue_t *q)
{
    assert(q != NULL);

    thrd_info_t th = {
        .init = NULL,
        .callback = reading_cb,
        .destroy = NULL,
        .context = (void *) q,
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
    int err;
    samp_t sampler;
    thrd_pool_t pool;
    thdqueue_t *queue;
    sampth_handler_t h;
    disp_t dispatcher;

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
    queue = thdqueue_new();
    if (sampth_subscribe(&h, &pool, &sampler, queue)) {
        thrd_err_t err = thrd_interr(&pool);

        ERR_FMT("Pool init: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }
    disp_init(&dispatcher, queue, (disp_dup_t)sampth_frameset_dup);

    if ((err = create_reading_thread(&pool, disp_new_hook(&dispatcher))) != 0) {
        thrd_err_t err = thrd_interr(&pool); 

        ERR_FMT("Adding first reader: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }
    if ((err = create_plotting_thread(&pool, disp_new_hook(&dispatcher))) != 0) {
        thrd_err_t err = thrd_interr(&pool); 

        ERR_FMT("Adding second reader: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    if (thrd_start(&pool)) {
        thrd_err_t err = thrd_interr(&pool);

        ERR_FMT("Starting init: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    sleep(60);
    DEBUG_MSG("Killing sampler after 60 secs");
    if (sampth_sendkill(h)) {
        DEBUG_MSG("...but I failed miserably");
    } else {
        DEBUG_MSG("...And I succeded in it");
    }

    thrd_destroy(&pool);
    samp_destroy(&sampler);
    disp_destroy(&dispatcher);

    exit(EXIT_SUCCESS);
}
