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
#include <assert.h>

#include "headers/plotting.h"
#include "headers/constants.h"
#include "headers/logging.h"

void plot_init (plot_t *p, size_t bufsize)
{
    plPlotter *plot;
    plPlotterParams *params;
    int err;

    params = pl_newplparams();
    assert(params);

    err = pl_setplparam(params, "BITMAPSIZE", PLOT_BITMAPSIZE);
    assert(err >= 0);
    err = pl_setplparam(params, "VANISH_ON_DELETE", "yes");
    assert(err >= 0);
    err = pl_setplparam(params, "USE_DOUBLE_BUFFERING", "yes");
    assert(err >= 0);

    plot = pl_newpl_r("X", NULL, NULL, stderr, params);
    assert(plot);

    pl_deleteplparams(params);

    err = pl_openpl_r(plot);
    assert(err >= 0);
    err = pl_space_r(plot, 0, PLOT_MIN_Y, bufsize, PLOT_MAX_Y);
    assert(err >= 0);
	err = pl_flinewidth_r(plot, 0.25);
    assert(err >= 0);
	err = pl_pencolorname_r(plot, "red");
    assert(err >= 0);

    p->plot = plot;
    p->circular = dlist_new();
    p->bufsize = bufsize;
    p->stored = 0;
}

static
void add_circular (plot_t *p, intptr_t val)
{
    p->circular = dlist_append(p->circular, (void *)val);
    if (p->stored < p->bufsize) {
        p->stored ++;
    } else {
        intptr_t phony;
        p->circular = dlist_pop(p->circular, (void **) &phony);
    }
}

static
void replot (plot_t *p)
{
    diter_t *iter;
    unsigned count;
    plPlotter *plot;

    iter = dlist_iter_new(&p->circular);
    count = 0;
    plot = p->plot;
    while (diter_hasnext(iter)) {
        int16_t value;

        value = (intptr_t) diter_next(iter);
        if (count == 0) {
            DEBUG_FMT("Moving cursor at <%i,%i>", 0, value);
            pl_move_r(plot, 0, value);
        } else {
            DEBUG_FMT("Line to <%i,%i>", count, value);
            pl_cont_r(plot, count, value);
        }
        
        count ++;
    }
    pl_endpath_r(plot);
    pl_erase_r(plot);
    dlist_iter_free(iter);
}

void plot_add_value (plot_t *p, int16_t val)
{
    add_circular(p, val);
    replot(p);
}

void plot_destroy (plot_t *p)
{
    pl_closepl_r(p->plot);
    pl_deletepl_r(p->plot);
    dlist_free(p->circular, NULL);
}

