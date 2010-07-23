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

#ifndef __defined_headers_sampth_h
#define __defined_headers_sampth_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/alsagw.h"
#include "headers/thrd.h"

#include <thdacav/thdacav.h>

typedef struct sampth_data * sampth_handler_t;

/** Data type for sampling set.
 *
 * This is the data produced by the thread and pushed to the output queue.
 *
 * @see sampth_subscribe().
 */
typedef struct {
    snd_pcm_uframes_t nframes;  /**< Number of stored frames_t objects */
    samp_frame_t *frames;       /**< Sequentially read objects */

} sampth_frameset_t;

/** Subscribe a sampling thread to a thread pool.
 *
 * @param handler A pointer where the sampling thread handler address will
 *                be stored;
 * @param pool The pool used for subscribing;
 * @param samp The sampler;
 * @param output The output queue in which data will be pushed.
 *
 * @see headers/alsagw.h.
 * @see headers/thrd.h.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
int sampth_subscribe (sampth_handler_t *handler, thrd_pool_t *pool,
                      const samp_t *samp, thdqueue_t *output);

/** Request sampling termination.
 *
 * This call terminates the running sampler and signals the data
 * termination on the output queue.
 *
 * @param handler The handler of the sampling thread.
 */
int sampth_sendkill (sampth_handler_t handler);

/** Destructor for data set.
 *
 * @param set The data set to be freed.
 */
void sampth_frameset_destroy (sampth_frameset_t *set);

/** Copy constructor for data set.
 *
 * @param set The data set to be copied.
 *
 * @return The newly allocated data set
 */
sampth_frameset_t * sampth_frameset_dup (const sampth_frameset_t *set);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_sampth_h

