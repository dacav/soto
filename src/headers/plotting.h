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

#ifndef __defined_headers_plotting_h
#define __defined_headers_plotting_h
#ifdef __cplusplus
extern "C" {
#endif

#include <dacav/dacav.h>
#include <plot.h>
#include <stdint.h>

#include "headers/alsagw.h"

typedef struct {
    plPlotter * plot;
    samp_frame_t * circular;
    size_t bufsize;
    size_t stored;
    unsigned cursor;
} plot_t;

void plot_init (plot_t *p, size_t bufsize);

void plot_add_frame (plot_t *p, samp_frame_t *frame);

void plot_destroy (plot_t *p);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_plotting_h

