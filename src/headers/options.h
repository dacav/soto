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
#include <dacav/dacav.h>
#include <stdbool.h>

/** @brief Option opaque type. */
typedef struct opts opts_t;

/** @brief Parse the command line options
 *
 * @note This function is supposed to work as interface with the shell,
 *       therefore errors are directly shown on stderr.
 *
 * @param argc Main's argument counter;
 * @param argv Main's argument vector.
 *
 * @return the options set on success, NULL on failure.
 */
opts_t * opts_parse (int argc, char * const argv[]);

/** @brief Destructor of options.
 *
 * @param o The options set to be destroyed.
 *
 */
static inline
void opts_destroy (opts_t *o)
{
    free(o);
}

/** @brief Getter for the audio device.
 *
 * @param o The options set.
 * @return The audio device.
 */
const char * opts_get_device (opts_t *o);

/** @brief Getter for the sampling rate.
 *
 * @param o The options set;
 * @return The sampling rate.
 */
unsigned opts_get_rate (opts_t *o);

/** @brief Getter for the minimum priority.
 *
 * @param o The options set;
 * @return The minimum priority.
 */
unsigned opts_get_minprio (opts_t *o);

/** @brief Getter for the execution time.
 *
 * Execution time is the actual duration, in seconds, of the
 * sampling/plotting phase.
 *
 * @param o The options set;
 * @return The execution time.
 */
unsigned opts_get_run_for (opts_t *o);

/** @brief Show the spectrum predicate. 
 *
 * @param o The options set.
 * @retval true If the program must show the spectrum.
 * @retval false If the program must not show the spectrum.
 */
bool opts_spectrum_shown (opts_t *o);

/** @brief Show the signal predicate. 
 *
 * @param o The options set.
 * @retval true If the program must show the spectrum.
 * @retval false If the program must not show the spectrum.
 */
bool opts_signal_shown (opts_t *o);

/** @brief Getter for the buffer scale.
 *
 * Buffer scale is a multiplicative factor that defines the proportion
 * between the alsa-defined sampling buffer size and the buffer where the
 * samples shall be stored.
 *
 * @param o The options set
 * @return The buffer scale
 */
unsigned opts_get_buffer_scale (opts_t *o);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_options_h

