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

#ifndef __defined_headers_alsasample_h
#define __defined_headers_alsasample_h
#ifdef __cplusplus
extern "C" {
#endif

#include <thdacav/thdacav.h>
#include <alsa/asoundlib.h>

#include <time.h>
#include <stdint.h>

typedef struct {
    const char *device;
    unsigned rate;
    unsigned nsamp;
    uint8_t channels;
} samp_info_t;

typedef enum {
    SAMP_ACCEPT_RATE = 1 << 0
} samp_policy_t;

typedef struct {
    snd_pcm_t *pcm;

    void *buffer;
    size_t nframes;
    struct timespec period;

    uint8_t status;
    int err;
} samp_t;

typedef enum {
    SAMP_ERR_LIBRARY  = 1 << 0,     /**< Library error */
    SAMP_ERR_RATE     = 1 << 1,     /**< Rate has been modified */
    SAMP_ERR_PERIOD   = 1 << 2,     /**< Period has been modified */
} samp_err_t;

/** Constructor for the sampler.
 *
 * @note the spec parameter may get side effects.
 *
 * @param s The sampler instance;
 * @param spec The specification;
 * @param accept The policy for specification adjustment;
 * @param out Output queue (for sampled data);
 * @return 0 on success, on failure.
 */
int samp_init (samp_t *s, const samp_info_t *spec, samp_policy_t accept);

/** Getter for the computed period.
 *
 * @param samp The sampler;
 * @return The time between reads.
 */
const struct timespec * samp_get_period (const samp_t *samp);

/** Get information on the error.
 *
 * After this call the internal error-keeping structure of the s gets
 * resetted.
 *
 * @see samp_err_t
 * @see samp_strerr
 *
 * @param s The sampler for which the last call returned error;
 * @return The error.
 */
samp_err_t samp_interr (samp_t *s);

/** Get error description string.
 *
 * @param err The error;
 * @return The internal error description for the errors, strerror(3)
 *         return value when err is SAMP_ERR_LIBRARY.
 */
const char * samp_strerr (samp_t *s, samp_err_t err);

void samp_destroy (samp_t *s);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_alsasample_h
