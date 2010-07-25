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

/** @file plotting.h
 *
 * This module restricts the access to plot.h and provides a clean
 * interface that allows to plot a representation composed of two
 * graphics.
 *
 */

#ifndef __defined_headers_plotting_h
#define __defined_headers_plotting_h
#ifdef __cplusplus
extern "C" {
#endif

#include <dacav/dacav.h>
#include <plot.h>
#include <stdint.h>

#include "headers/alsagw.h"

/** Plotting system.
 *
 * The plotter shows two graphics, one for each of the two channels.
 */
typedef struct {
    plPlotter * plot;           /**< Libplot handler; */
    samp_frame_t * circular;    /**< Circular buffer for plotting; */
    size_t bufsize;             /**< Size of the circular buffer; */
    size_t stored;              /**< Number of element stored in the
                                 *   buffer */
    unsigned cursor;            /**< Current position into the buffer. */
} plot_t;

/** Plotter constructor.
 *
 * This function spawns a X11 window on which the plot will be displayed.
 *
 * @param p The plotter to be initialized;
 * @param bufsize The number of backward values showed during plotting.
 */
void plot_init (plot_t *p, size_t bufsize);

/** Add a new frame to plotting.
 *
 * This function directly updates the X11 window with the two new values
 * contained into the second parameter.
 *
 * @param p The plotter;
 * @param frame The new frame to be buffered.
 */
void plot_add_frame (plot_t *p, samp_frame_t *frame);

/** Plotter Destructor
 *
 * @param p The plotter to be destroyed.
 */
void plot_destroy (plot_t *p);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_plotting_h

