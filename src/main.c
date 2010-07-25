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
#include "headers/plotthread.h"
#include "headers/dispatch.h"

static
void samp_info_by_opts (samp_info_t *info, opts_t *opts)
{
    info->device = opts->device;
    info->rate = opts->rate;
    info->nsamp = opts->nsamp;
    info->channels = opts->mode == MONO ? 1 : 2;
}

int main (int argc, char **argv)
{
    opts_t opts;
    int err;
    samp_t sampler;
    samp_info_t sampinfo;
    thrd_pool_t pool;
    thdqueue_t *queue;
    sampth_handler_t h;
    disp_t dispatcher;

    /* Parsing options */
    if (opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    /* Threadpool initialization */
    thrd_init(&pool, opts.minprio);

    /* Build sampling information by reading options */
    samp_info_by_opts(&sampinfo, &opts);

    /* Init sampling system */
    if (samp_init(&sampler, &sampinfo, opts.policy)) {
        samp_err_t err = samp_interr(&sampler);

        ERR_FMT("Sampler init: %s", samp_strerr(&sampler, err));
        exit(EXIT_FAILURE);
    }

    /* Build communication queue */
    queue = thdqueue_new();

    /* Subscribe sampling thread, it will be enabled later. */
    if (sampth_subscribe(&h, &pool, &sampler, queue)) {
        thrd_err_t err = thrd_interr(&pool);

        ERR_FMT("Pool init: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    /* Init the sampling dispatching system */
    disp_init(&dispatcher, queue, (disp_dup_t)sampth_frameset_dup);

    /* Subscribe the average + plot system */
    if ((err = plotth_subscribe(&pool, disp_new_hook(&dispatcher),
                                &sampinfo)) != 0) {
        thrd_err_t err = thrd_interr(&pool); 

        ERR_FMT("Adding second reader: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    /* This is not elegant, but I have to deliver */
    LOG_MSG("This program will start in two seconds and terminate after 1 minute");
    sleep(2);

    /* Starting the pool */
    if (thrd_start(&pool)) {
        thrd_err_t err = thrd_interr(&pool);

        ERR_FMT("Starting init: %s", thrd_strerr(&pool, err));
        exit(EXIT_FAILURE);
    }

    sleep(60);
    LOG_MSG("Sending termination signal.");
    if (sampth_sendkill(h)) {
        LOG_MSG("...but I failed miserably");
    } else {
        LOG_MSG("...And I succeded in it");
    }

    thrd_destroy(&pool);
    samp_destroy(&sampler);
    disp_destroy(&dispatcher);

    exit(EXIT_SUCCESS);
}
