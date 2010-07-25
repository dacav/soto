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

/** @file options.h
 *
 * This module provieds an options parsing mechanism based on getopt.
 */


#ifndef __defined_headers_options_h
#define __defined_headers_options_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/alsagw.h"

#include <alsa/asoundlib.h>
#include <dacav/dacav.h>

#include <stdint.h>
#include <time.h>

/** Option keeping structure.
 *
 * @note The field opts_t::minpio will be added to the value returned by
 *       the sched_get_priority_min() system call.
 */
typedef struct {

    /* ALSA related options */

    const char *device;         /**< PCM device */

    enum {
        MONO,                   /**< One channel */
        STEREO                  /**< Two channels */
    } mode;                     /**< Number of input channels */

    unsigned rate;              /**< Sample rate; */
    size_t nsamp;               /**< Number of samples buffered; */
    size_t nplot;               /**< Number of plotters; */
    
    /** Defines the policy of the initialization, @see samp_policy_t */
    samp_policy_t policy;

    /** Minimum priority value to be used. This will be added to the
     *  result of the sched_get_priority_min() syscall.
     */
    int minprio;

} opts_t;

/** Parse the command line options
 *
 * @param opts The structure holding the options;
 * @param argc Main's argument counter;
 * @param argv Main's argument vector;
 * @return 0 on success, non-zero on fail.
 */
int opts_parse (opts_t *opts, int argc, char * const argv[]);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_options_h

