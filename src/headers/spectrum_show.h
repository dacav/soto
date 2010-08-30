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

/** @file spectrum_show.h */
/** @addtogroup BizSpectrum */
/*@{*/


#ifndef __defined_headers_specthread_h
#define __defined_headers_specthread_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/thrd.h"
#include "headers/sampthread.h"
#include "headers/plotting.h"
#include "alsagw.h"

/** @brief Parameter structure for specth_subscribe().
 *
 * Provides the four graphics to write in.
 */
typedef struct {
    plotgr_t *r0;   /**< Real part of channel 0 */
    plotgr_t *i0;   /**< Imaginary part of channel 0 */
    plotgr_t *r1;   /**< Real part of channel 1 */
    plotgr_t *i1;   /**< Imaginary part of channel 1 */
} specth_graphics_t;

/** @brief Subscribe a direct thread to the given pool.
 *
 * The sampling object (alsagw_t) must not necessarly be already created
 * when this function is called, however the samp parameter must contain
 * the same definition, since it will determine the period definition of
 * the realtime thread.
 *
 * @param handle Thea address of a pointer where the plotting thread
 *               handle address will be stored;
 * @param pool The pool to which the sampler will be subscribed;
 * @param sampth The handle of the sampler thread;
 * @param graphs A structure containing pointers to the graphs.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
const thrd_rtstats_t * specth_subscribe (genth_t **handle,
                                         thrd_pool_t *pool,
                                         genth_t *sampth,
                                         const specth_graphics_t *graphs);

/*@}*/

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_specthread_h

