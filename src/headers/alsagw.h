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

/** @file alsagw.h
 *
 * This module provieds an interface to the alsa system.
 */

#ifndef __defined_headers_alsagw_h
#define __defined_headers_alsagw_h
#ifdef __cplusplus
extern "C" {
#endif

#include <thdacav/thdacav.h>
#include <alsa/asoundlib.h>

#include <time.h>
#include <stdint.h>

/** Specification used by audiocard initialization.
 *
 * @see samp_init.
 */
typedef struct {
    const char *device;         /**< Device name; */
    unsigned rate;              /**< Required sampling rate; */
    unsigned nsamp;             /**< Number of samples per buffer; */
    uint8_t channels;           /**< Number of channels. */
} samp_info_t;

/** Policy for parameter changing.
 *
 * Alsa will probably modify your parameter in order to adapt it to the
 * audiocard. The following flags allow you to better control this
 * behaviour.
 */
typedef enum {
    SAMP_ACCEPT_RATE = 1 << 0   /**< Accept the suggested sampling rate */
} samp_policy_t;

/** Sampling system descriptor.
 *
 * This must be allocated but managed trough proper functions.
 *
 * @see samp_init.
 */
typedef struct {
    snd_pcm_t *pcm;             /**< Alsa handler */

    /** Number of frames that can be buffered with the given sampling
     *  specification.
     *
     * This is an auxiliary information that shall be used by other
     * modules and read trough the proper getter.
     *
     * @see samp_info_t.
     * @see samp_get_period.
     */
    snd_pcm_uframes_t nframes;

    /** Number of frames that can be buffered with the given sampling
     *  specification.
     *
     * This is an auxiliary information that shall be used by other
     * modules and read trough the proper getter.
     *
     * @see samp_info_t.
     * @see samp_get_period.
     */
    struct timespec period;

    uint8_t status;             /**< Status flags, for internal use */
    int err;                    /**< Library error, for internal use */
} samp_t;

/** Error type for sampling */
typedef enum {
    SAMP_ERR_LIBRARY  = 1 << 0,     /**< Library error; */
    SAMP_ERR_RATE     = 1 << 1,     /**< Rate has been modified; */
    SAMP_ERR_PERIOD   = 1 << 2,     /**< Period has been modified. */
} samp_err_t;

/** Data type for frames.
 *
 * @note Currently this project provides only sampling with format
 *       SND_PCM_FORMAT_S16_LE with interleaved access. Further
 *       modification with parametrized choice of format/interleving shall
 *       for sure modify this structure.
 *
 */
typedef struct {
    int16_t ch0;   /**< First channel; */
    int16_t ch1;   /**< Second channel. */
} samp_frame_t;

/** Constructor for the sampler.
 *
 * @note the spec parameter may get side effects.
 *
 * @param s The sampler instance;
 * @param spec The specification;
 * @param accept The policy for specification adjustment.
 *
 * @see samp_policy_t.
 *
 * @return 0 on success, another value on failure.
 */
int samp_init (samp_t *s, const samp_info_t *spec, samp_policy_t accept);

/** Getter for the computed period.
 *
 * @param samp The sampler;
 * @return The time between reads.
 */
const struct timespec * samp_get_period (const samp_t *samp);

/** Getter for the buffer size
 *
 * @see frame_t.
 *
 * @param samp The sampler;
 * @return The buffer size in frames.
 */
snd_pcm_uframes_t samp_get_nframes (const samp_t *samp);

/** Getter for the ALSA pcm.
 *
 * @param samp The sampler;
 * @return The pcm.
 */
snd_pcm_t * samp_get_pcm (const samp_t *samp);

/** Get information on the error.
 *
 * After this call the internal error-keeping structure of the s gets
 * resetted.
 *
 * @see samp_err_t
 * @see samp_strerr
 *
 * @param s The sampler for which the last call returned error.
 *
 * @return The error.
 */
samp_err_t samp_interr (samp_t *s);

/** Get error description string.
 *
 * @param s The sampler;
 * @param err The error obtained by samp_interr.
 *
 * @return The internal error description for the errors. If err is
 *         samp_err_t::SAMP_ERR_LIBRARY the return value of strerror(3)
 *         will be returned instead.
 */
const char * samp_strerr (samp_t *s, samp_err_t err);

/** Destroyer.
 *
 * @param s The sampler to be destroyed.
 */
void samp_destroy (samp_t *s);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_alsagw_h

