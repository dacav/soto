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

/** @file sampthread.h
 *
 * This module provieds a clean interface to pool (headers/thrd.h)
 * subscription of the thread in charge of doing the sampling phase.
 *
 */

/** @file sampthread.h */

#ifndef __defined_headers_sampth_h
#define __defined_headers_sampth_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/alsagw.h"
#include "headers/genthrd.h"

/** Subscribe a sampling thread to a thread pool.
 *
 * @param handler Thea address of a pointer where the sampling thread
 *                handler address will be stored;
 * @param pool The pool used for subscribing;
 * @param samp The sampler;
 *
 * @see headers/alsagw.h.
 * @see headers/thrd.h.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
int sampth_subscribe (genth_t **handler, thrd_pool_t *pool,
                      const samp_t *samp, size_t scaling_factor);

/** Getter for the size of the reading buffer.
 *
 * @param handler The handler of the sampling thread.
 *
 * @return The size of the buffer in frames.
 */
snd_pcm_uframes_t sampth_get_size (const genth_t *handler);

/** Thread-safe getter for the content of the reading buffer.
 *
 * The function assumes the buffer to be at least N frames long, where N
 * is the value returned by the sampth_get_size() function.
 *
 * @param handler The sampling thread which buffer shall be read;
 * @param buffer The buffer where the data shall be stored.
 */
void sampth_get_samples (genth_t *handler, samp_frame_t buffer[]);

/** Getter for the correct reading period for the buffer.
 *
 * @param handler The handler of the sampling thread.
 *
 * @return The time required by the thread to fill in its internal
 *         buffer.
 */
const struct timespec * sampth_get_period (const genth_t *handler);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_sampth_h

