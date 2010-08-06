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
 * This header provides various constant definitions.
 */

#ifndef __defined_headers_constants_h
#define __defined_headers_constants_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** @brief Number of seconds per nanoseconds. */
#define SECOND_NS               1000000000UL

/** @brief Startup delay for sampling thread, seconds. */
#define SAMP_STARTUP_DELAY_SEC  0

/** @brief Startup delay for sampling thread, nanoseconds. */
#define SAMP_STARTUP_DELAY_nSEC 500000

/** @brief Period for direct plotting thread, seconds. */
#define PLOT_PERIOD_SEC        0

/** @brief Period for direct plotting thread, nanoseconds.
 *
 * This value corresponds to 1.0 sec / 28, namely a suitable frequency for
 * the human eye to notice the plot updating.
 */
#define PLOT_PERIOD_nSEC        35714286     

/** @brief Proportion divisor between sampling thread period and sampling wait in
 * case of failure. This will be multiplied by period in sampthread.c.
 */
#define ALSA_WAIT_PROPORTION    2

/** @brief Size of the graphical window used for plotting. */
#define PLOT_BITMAPSIZE         "400x250"

/** @brief Color of the plotting line. */
#define PLOT_LINECOLOR          "green"

/** @brief Color of the plotting background. */
#define PLOT_BGCOLOR            "black"

/** @brief Minimum plotable value. */
#define PLOT_MIN_Y              INT16_MAX

/** @brief Maximum plotable value. */
#define PLOT_MAX_Y              INT16_MIN

/** @brief Multiplication factor (signal_show.c).
 *
 * The period of the plotter will be a multiple of the sampling period
 * w.r.t. this constant.
 */
#define PLOT_PERIOD_TIMES       10

/** @brief Number of sampling used in averaging samples. */
#define PLOT_AVERAGE_LEN        50

/** @brief Default rate used in the options module. */
#define DEFAULT_RATE            44100

/** @brief Default device used in the options module. */
#define DEFAULT_DEVICE          "hw:0,0"

/** @brief Default proportion between sampling buffer and read buffer */
#define DEFAULT_BUFFER_SCALE    50

/** @brief Default time of execution for the buffer */
#define DEFAULT_RUN_FOR         0

/** @brief Default minimum priority used in the options module
 *
 * @see opts_t::minprio.
 */
#define DEFAULT_MINPRIO         0

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_constants_h

