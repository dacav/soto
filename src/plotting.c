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
#include <stdlib.h>

#include "headers/plotting.h"
#include "headers/constants.h"
#include "headers/logging.h"

static
plPlotter * init_libplot (size_t bufsize)
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
	err = pl_linewidth_r(plot, 1);
    assert(err >= 0);
	err = pl_pencolorname_r(plot, "black");
    assert(err >= 0);

    return plot;
}


void plot_init (plot_t *p, size_t bufsize)
{
    p->plot = init_libplot(bufsize);

    p->circular = malloc(bufsize * sizeof(samp_frame_t));
    assert(p->circular);
    p->bufsize = bufsize;
    p->stored = p->cursor = 0;
}

static
void draw_lines (plPlotter *plot, unsigned count, samp_frame_t *from,
                 samp_frame_t *to)
{
    pl_move_r(plot, count - 1, from->ch0 + PLOT_OFFSET_UP);
    pl_cont_r(plot, count, to->ch0 + PLOT_OFFSET_UP);
    pl_endpath_r(plot);

    pl_move_r(plot, count - 1, from->ch1 + PLOT_OFFSET_DOWN);
    pl_cont_r(plot, count, to->ch1 + PLOT_OFFSET_DOWN);
    pl_endpath_r(plot);
}

static
void scan (plot_t *p)
{
    unsigned i, n, count;
    samp_frame_t *circ;

    circ = p->circular;
    n = p->stored;
    count = 1;

    i = p->cursor + 1;
    while (count < n && i < p->bufsize) {
        draw_lines(p->plot, count ++, &circ[i - 1], &circ[i]);
        i ++;
    }
    if (count < n) {
        draw_lines(p->plot, count ++, &circ[p->bufsize - 1], &circ[0]);
    }
    i = 1;
    while (count < n && i < p->cursor) {
        draw_lines(p->plot, count ++, &circ[i - 1], &circ[i]);
        i ++;
    }
}

static
void insert (plot_t *plot, samp_frame_t *pair)
{
    unsigned cur;

    cur = plot->cursor;
    plot->circular[cur ++] = *pair;
    plot->cursor = cur >= plot->bufsize ? 0 : cur;
    if (plot->stored ++ > plot->bufsize) {
        plot->stored = plot->bufsize;
    }
}

void plot_add_frame (plot_t *p, samp_frame_t *frame)
{
    insert(p, frame);
    scan(p);
    pl_erase_r(p->plot);
}

void plot_destroy (plot_t *p)
{
    pl_closepl_r(p->plot);
    pl_deletepl_r(p->plot);
    free(p->circular);
}

