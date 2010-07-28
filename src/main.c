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

#include "headers/alsagw.h"
#include "headers/logging.h"
#include "headers/sampthread.h"
#include "headers/thrd.h"
#include "headers/plotting.h"

int main (int argc, char **argv)
{
    plot_t *p;

    plotgr_t *g0, *g1, *g2, *g3;

    p = plot_new(4, 10);

    g0 = plot_new_graphic(p);
    g1 = plot_new_graphic(p);
    g2 = plot_new_graphic(p);
    g3 = plot_new_graphic(p);

    for (;;) {
    	int i;

    	for (i = 0; i < 10; i ++) {
    		plot_graphic_set(g0, i, (1<<10) * rand());
    		plot_graphic_set(g1, i, (1<<9) * rand());
    		plot_graphic_set(g2, i, (1<<8) * rand());
    		plot_graphic_set(g3, i, (1<<8) * rand());
    	}
    	plot_redraw(p);
    }

    plot_destroy(p);
}

#if 0
    samp_t *samp;
    sampth_handler_t sampth;
    thrd_pool_t *pool;
    int err;

    pool = thrd_new(0);
    samp = samp_new("hw:0,0", 96000, 2, &err);
    if (samp == NULL) {
        LOG_FMT("SUP? %s\n", snd_strerror(err));
    }

    sampth_subscribe(&sampth, pool, samp, 10);
    thrd_start(pool);

    sleep(10);

    sampth_sendkill(sampth);
    samp_destroy(samp);
    thrd_destroy(pool);
}
#endif
