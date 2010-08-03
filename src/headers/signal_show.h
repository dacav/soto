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

/** @file signal_show.h
 *
 * This module provieds the subscription through genthrd of a
 * thread in charge of displaying the raw data.
 *
 */

#ifndef __defined_headers_showthread_h
#define __defined_headers_showthread_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/thrd.h"
#include "headers/sampthread.h"
#include "headers/plotting.h"
#include "alsagw.h"

/** Subscribe a direct thread to the given pool.
 *
 * The sampling object (samp_t) must not necessarly be already created
 * when this function is called, however the samp parameter must contain
 * the same definition, since it will determine the period definition of
 * the realtime thread.
 *
 * @param handle Thea address of a pointer where the plotting thread
 *               handle address will be stored;
 * @param pool The pool to which the sampler will be subscribed;
 * @param sampth The handle of the sampler thread;
 * @param g0 The graphic where the channel 0 data will be shown;
 * @param g1 The graphic where the channel 1 data will be shown.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
int showth_subscribe (genth_t **handle, thrd_pool_t *pool,
                      genth_t *sampth, plotgr_t *g0, plotgr_t *g1);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_showthread_h

