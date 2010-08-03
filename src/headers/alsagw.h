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

#include <alsa/asoundlib.h>

#include <time.h>
#include <stdint.h>

/** Sampling system descriptor.
 *
 * This must be allocated but managed trough proper functions.
 *
 * @see samp_init.
 */
typedef struct samp samp_t;

/** Sample type: just a pair of int16.
 *
 * For sake of simplicity the program manages audio streams in the format
 * S16 LE PCM (Signed 16-bit Little-Endian Pulse Code Modulation).
 */
typedef struct {
    int16_t ch0;    /**< Channel 0; */
    int16_t ch1;    /**< Channel 1. */
} samp_frame_t;

/** Constructor for the sampler.
 *
 * @param device The alsa device (e.g. "hw0:0");
 * @param rate The sampling rate;
 * @param channels Number of channels. Allowed values: 1 or 2.
 * @param err The pointer where the library error, if any, will be stored.
 *
 * @note In case of error, the snd_strerror() provided by Alsa can be used
 *       on err.
 *
 * @return The newly allocated sampler or NULL on error.
 */
samp_t * samp_new (const char *device, unsigned rate,
                   uint8_t channels, int *err);

/** Getter for the computed period.
 *
 * @param samp The sampler;
 * @return The time between reads.
 */
const struct timespec * samp_get_period (const samp_t *samp);

/** Getter for the size in frames of the buffer.
 *
 * This module is not in charge to allocate the buffer: this primitive
 * simply returns the size of the buffer that an external module must use.
 *
 * @param samp The sampler;
 * @return The buffer size in frames.
 */
snd_pcm_uframes_t samp_get_nframes (const samp_t *samp);

/** Getter for the ALSA pcm.
 *
 * @param samp The sampler.
 * @return The pcm handler.
 */
snd_pcm_t * samp_get_pcm (const samp_t *samp);

/** Getter for the sampling rate.
 *
 * @param samp The sampler.
 * @return The sampling rate.
 *
 * @note Alsa may decide to modify the sample rate given by the
 *       constructor in order to fit soundcard specification, therefore
 *       it's wise to relay on the rate returned by this function, instead
 *       of using the one provided as parameter for samp_new().
 */
unsigned samp_get_rate (const samp_t *samp);

/** Destroyer.
 *
 * @param s The sampler to be destroyed.
 */
void samp_destroy (samp_t *s);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_alsagw_h

