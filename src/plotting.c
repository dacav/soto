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

#include <pthread.h>

#include "headers/plotting.h"
#include "headers/constants.h"
#include "headers/logging.h"

struct plot {
    size_t ngraphics;   /* Number of graphics in the window; */
    plotgr_t *graphics; /* Array of graphics; */
    size_t used;        /* Number of graphics in use; */
    unsigned max_x;     /* Maximum value for the x axis; */

    plPlotter *handle;  /* libplot handle. */
};

struct graphic {
    plot_t *main_plot;  /* Pointer to the main plot; */
    int y_offset;       /* Vertical offset of this graphic; */
    int16_t *values;    /* Stored values that gets modified; */

    /* Two kind of threads may access this: the one which is updating the
     * plot and the possibly multiple ones updating the data. */
    pthread_mutex_t lock;
};

/* Reentrant initialization for libplot. Thanks to the guy who fixed the
 * libplot.
 */
static
plPlotter * init_libplot (size_t nplots, int max_x)
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
    err = pl_space_r(plot, 0, PLOT_MIN_Y * nplots, max_x,
                     PLOT_MAX_Y * nplots);
    assert(err >= 0);
	err = pl_linewidth_r(plot, 1);
    assert(err >= 0);
	err = pl_pencolorname_r(plot, "black");
    assert(err >= 0);

    return plot;
}

plot_t * plot_new (size_t n, unsigned max_x)
{
    plot_t *p;

    assert(max_x > 1);
    p = malloc(sizeof(plot_t));
    assert(p);
    p->graphics = calloc(n, sizeof(plotgr_t));
    assert(p->graphics);
    p->ngraphics = n;
    p->used = 0;
    p->handle = init_libplot(n, max_x);
    p->max_x = max_x;
    return p;
}

plotgr_t * plot_new_graphic (plot_t *p)
{
    plotgr_t *g;
    int used;

    if (p->used >= p->ngraphics) {
        return NULL;
    }
    used = p->used;
    g = p->graphics + used; 
    p->used ++;

    g->main_plot = p;
    g->y_offset = used * (PLOT_MAX_Y - PLOT_MIN_Y)
                  + (p->ngraphics - 1) * PLOT_MIN_Y;
    g->values = calloc(p->max_x, sizeof(int16_t));
    pthread_mutex_init(&g->lock, NULL);

    return g;
}

static
void draw_lines (plPlotter *plot, int16_t vals[], size_t nvals,
                 int offset)
{
    int i;

    pl_move_r(plot, 0, vals[0] + offset);
    for (i = 1; i < nvals; i ++) {
        pl_cont_r(plot, i, vals[i] + offset);
    }
    pl_endpath_r(plot);
}

void plot_redraw(plot_t *p)
{
    int i;
    plotgr_t *g;

    g = p->graphics;
    for (i = 0; i < p->used; i ++) {
        pthread_mutex_lock(&g->lock);
        draw_lines(p->handle, g->values, p->max_x,
                   g->y_offset);
        pthread_mutex_unlock(&g->lock);
        g ++;
    }
    pl_erase_r(p->handle);
}

void plot_graphic_set (plotgr_t *g, unsigned pos, int16_t val)
{
    plot_t *p = g->main_plot;

    assert(p->max_x > pos);

    pthread_mutex_lock(&g->lock);
    g->values[pos] = val;
    pthread_mutex_unlock(&g->lock);
}

void plot_destroy (plot_t *p)
{
    int i;
    plotgr_t *graphics;

    pl_closepl_r(p->handle);
    pl_deletepl_r(p->handle);
    graphics = p->graphics;
    for (i = 0; i < p->used; i ++) {
        pthread_mutex_destroy(&graphics[i].lock);
        free(graphics[i].values);
    }
    free(p->graphics);
    free(p);
}

