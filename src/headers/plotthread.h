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

/*

   This module provieds a clean interface to pool (headers/thrd.h)
   subscription of the thread in charge of doing the data elaboration
   phase.

 */

#ifndef __defined_headers_plotthread_h
#define __defined_headers_plotthread_h
#ifdef __cplusplus
extern "C" {
#endif

#include <thdacav/thdacav.h>

#include "headers/thrd.h"
#include "alsagw.h"

/** Subscribe a plotting thread to the given pool.
 *
 * The sampling object (samp_t) must not necessarly be already created
 * when this function is called, however the samp parameter must contain
 * the same definition, since it will determine the period definition of
 * the realtime thread.
 *
 * @param pool The pool to which the sampler will be subscribed;
 * @param input The queue feeding the plotter with sampth_frameset_t
 *              objects;
 * @param samp The specification used as parameter for samp_init().
 *
 */
int plotth_subscribe (thrd_pool_t *pool, thdqueue_t *input,
                      const samp_info_t *samp);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_plotthread_h

