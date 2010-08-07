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

/** @file alsagw.h */
/** @addtogroup BizAlsaGw */
/*@{*/

#ifndef __defined_headers_alsagw_h
#define __defined_headers_alsagw_h
#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>

#include <time.h>
#include <stdint.h>

/** @brief Opaque type for the sampling system descriptor.
 *
 * The alsagw_new() function allocates and intializes a new descriptor.
 */
typedef struct samp alsagw_t;

/** @brief Sample type: just a pair of int16.
 *
 * For sake of simplicity the program manages audio streams in the format
 * S16 LE PCM (Signed 16-bit Little-Endian Pulse Code Modulation).
 */
typedef struct {
    int16_t ch0;    /**< Channel 0; */
    int16_t ch1;    /**< Channel 1. */
} alsagw_frame_t;

/** @brief Constructor for the sampler.
 *
 * @param device The alsa device (e.g. "hw0:0");
 * @param rate The sampling rate (e.g. 44100);
 * @param err The pointer where, if needed, library error will be stored.
 *
 * @note In case of error, the snd_strerror() provided by Alsa can be used
 *       on the integer pointed by err.
 *
 * @return The newly allocated sampler.
 * @retval NULL if something went wrong (in which case check err).
 */
alsagw_t * alsagw_new (const char *device, unsigned rate, int *err);

/** @brief Getter for the computed period.
 *
 * @param samp The sampler.
 * @return The time between reads.
 */
const struct timespec * alsagw_get_period (const alsagw_t *samp);

/** @brief Getter for the size in frames of the buffer.
 *
 * This module is not in charge to allocate the buffer: this primitive
 * simply returns the size of the buffer that an external module must use.
 *
 * @param samp The sampler;
 * @return The buffer size in frames.
 */
snd_pcm_uframes_t alsagw_get_nframes (const alsagw_t *samp);

/** @brief Semi-blocking read of a sample.
 *
 * This function automatically recovers xruns and errors.
 *
 * @param samp The sampler;
 * @param buffer The destination buffer;
 * @param bufsize The destination buffer's size;
 * @param maxwait The maximum blocking time (in nanoseconds);
 *
 * @return The number of read frames or a negative error code in case of
 *         failure.
 *
 * @note In case of failure, please relay on snd_strerr() in order to
 *       determine what's going on.
 *
 * @warning By providing a negative value for maxwait we allow Alsa to
 *          wait until the resource is available. Namely this will make
 *          this function blocking.
 */
int alsagw_read (alsagw_t *samp, alsagw_frame_t *buffer,
                 snd_pcm_uframes_t bufsize, int maxwait);

/** @brief Getter for the sampling rate.
 *
 * @param samp The sampler.
 * @return The sampling rate.
 *
 * @note This should be the same rate you gave as input for alsagw_new(),
 *       however Alsa may decide to modify the sample rate given by the
 *       constructor in order to fit soundcard specification. The wise
 *       programmer relies on the rate returned by this function, instead
 *       of using the one provided as parameter for alsagw_new().
 */
unsigned alsagw_get_rate (const alsagw_t *samp);

/** @brief Sampler destroyer.
 *
 * @warning Please do not forget to disable any thread using the sampler
 *          before using this function.
 *
 * @param s The sampler to be destroyed.
 */
void alsagw_destroy (alsagw_t *s);

/*@}*/

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_alsagw_h

