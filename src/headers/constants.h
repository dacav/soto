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

/** @file constants.h
 *
 *  This header provides various constant definitions.
 */

// TODO: Configuration via autotools.

#ifndef __defined_headers_constants_h
#define __defined_headers_constants_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** Number of seconds per nanoseconds. */
#define SECOND_NS 1000000000UL

/** Startup delay for all threads, seconds. */
#define STARTUP_DELAY_SEC       0

/** Startup delay for all threads, nanoseconds. */
#define STARTUP_DELAY_nSEC      500000

/** Proportion divisor between sampling thread period and sampling wait in
 * case of failure. This will be multiplied by period in sampthread.c.
 */
#define ALSA_WAIT_PROPORTION    2

/** Size of the graphical window used for plotting. */
#define PLOT_BITMAPSIZE         "500x300"

/** Minimum plotable value. */
#define PLOT_MIN_Y              (2 * INT16_MAX)

/** Maximum plotable value. */
#define PLOT_MAX_Y              (2 * INT16_MIN)

/** Offset from canvas center of first plot. */ 
#define PLOT_OFFSET_UP          INT16_MAX

/** Offset from canvas center of second plot. */ 
#define PLOT_OFFSET_DOWN        INT16_MIN

/** Multiplication factor (plotthread.c).
 *
 * The period of the plotter will be a multiple of the sampling period
 * w.r.t. this constant.
 */
#define PLOT_PERIOD_TIMES       10

/** Number of sampling used in averaging samples. */
#define PLOT_AVERAGE_LEN        50

/** Default rate used in the options module. */
#define DEFAULT_RATE            44100

/** Default device used in the options module. */
#define DEFAULT_DEVICE          "hw:0,0"

/** Default minimum priority used in the options module
 *
 * @see opts_t::minprio.
 */
#define DEFAULT_MINPRIO         0

/** Default number of plotting windows. */
#define DEFAULT_PLOTS_NUMBER     1

/** Proportion between sampling and plotting periods. */
#define DEFAULT_PERIOD_SLOTS    30

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_constants_h

