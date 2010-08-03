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

/** @file plotthread.h
 *
 * This module provieds the subscription through genthrd of a
 * thread in charge of doing the data elaboration phase.
 *
 */

#ifndef __defined_headers_plotthread_h
#define __defined_headers_plotthread_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/genthrd.h"
#include "headers/sampthread.h"
#include "headers/plotting.h"
#include "alsagw.h"

/** Subscribe a plotting thread to the given pool.
 *
 * @param handle Thea address of a pointer where the plotting thread
 *               handle address will be stored;
 * @param pool The pool to which the sampler will be subscribed;
 * @param plot The plotter instance.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
int plotth_subscribe (genth_t **handle, thrd_pool_t *pool,
                      plot_t *plot);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_plotthread_h

