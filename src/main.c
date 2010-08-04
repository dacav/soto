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

#include <alsa/asoundlib.h>

#include "headers/alsagw.h"
#include "headers/logging.h"
#include "headers/sampthread.h"
#include "headers/thrd.h"
#include "headers/plotting.h"
#include "headers/plotthread.h"
#include "headers/signal_show.h"
#include "headers/spectrum_show.h"

int main (int argc, char **argv)
{
    samp_t *samp;
    genth_t *sampth;
    thrd_pool_t *pool;
    plot_t *plot, *sigplot;
    genth_t *plotth, *sigplotth;
    genth_t *showth, *sigshowth;
    specth_graphics_t spec_graphs;
    long long unsigned miss;
    int err;

    pool = thrd_new(0);
    samp = samp_new("hw:0,0", 44100, 2, &err);
    if (samp == NULL) {
        LOG_FMT("Building samp: %s\n", snd_strerror(err));
    }

    if (sampth_subscribe(&sampth, pool, samp, 10)) {
        thrd_err_t err = thrd_interr(pool);
        LOG_FMT("Unable to startup: %s", thrd_strerr(pool, err));
        exit(EXIT_FAILURE);
    }

    plot = plot_new(4, sampth_get_size(sampth));
    if (plotth_subscribe(&plotth, pool, plot)) {
        thrd_err_t err = thrd_interr(pool);
        LOG_FMT("Unable to start plotting thread 1: %s",
                thrd_strerr(pool, err));
        exit(EXIT_FAILURE);
    }
    sigplot = plot_new(2, sampth_get_size(sampth));
    if (plotth_subscribe(&sigplotth, pool, sigplot)) {
        thrd_err_t err = thrd_interr(pool);
        LOG_FMT("Unable to start plotting thread 1: %s",
                thrd_strerr(pool, err));
        exit(EXIT_FAILURE);
    }
 
    spec_graphs.r0 = plot_new_graphic(plot);
    spec_graphs.i0 = plot_new_graphic(plot);
    spec_graphs.r1 = plot_new_graphic(plot);
    spec_graphs.i1 = plot_new_graphic(plot);
    if (specth_subscribe(&showth, pool, sampth,
                         &spec_graphs)) {
        LOG_FMT("Unable to start spectrum plotter: %s",
                thrd_strerr(pool, thrd_interr(pool)));
        exit(EXIT_FAILURE);
    }

    if (showth_subscribe(&sigshowth, pool, sampth,
                         plot_new_graphic(sigplot),
                         plot_new_graphic(sigplot))) {
        LOG_FMT("Unable to start signal plotter: %s",
                thrd_strerr(pool, thrd_interr(pool)));
        exit(EXIT_FAILURE);
    }

    if (thrd_start(pool)) {
        thrd_err_t err = thrd_interr(pool);
        LOG_FMT("Unable to startup: %s", thrd_strerr(pool, err));
        exit(EXIT_FAILURE);
    }

    sleep(100);

    genth_sendkill(plotth);
    genth_sendkill(sigplotth);
    genth_sendkill(sigshowth);
    genth_sendkill(showth);
    genth_sendkill(sampth);

    plot_destroy(plot);
    samp_destroy(samp);
    thrd_destroy(pool, &miss);

    LOG_FMT("We had %llu deadline misses", miss);

    exit(EXIT_SUCCESS);
}
