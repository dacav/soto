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

#ifndef __defined_headers_sampthread_h
#define __defined_headers_sampthread_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/alsasample.h"
#include "headers/thrd.h"

#include <thdacav/thdacav.h>

int sampthread_create (thrd_pool_t *pool, const samp_t *samp,
                       thdqueue_t *output);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_sampthread_h

