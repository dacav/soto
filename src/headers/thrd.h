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

   This module provieds a real-time thread pool with periodic threads
   managed in a Rate Monotonic flavour.

   Each Thread can be specified with a reentrant callback (which can be
   provided with a user private data), a period of activation and a delay
   for the first activation.

 */

#ifndef __defined_headers_thrd_h
#define __defined_headers_thrd_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <dacav/dacav.h>
#include <time.h>

#include "headers/logging.h"

typedef int (* callback_t) (void *context);

/** User definition for the thread.
 *
 * This is used as argument for the thrd_init function.
 */
typedef struct {
    
    /** Callback of the thread
     *
     * @param context The specified user data;
     * @return zero in order to kill the thread; non-zero otherwise.
     */
    callback_t callback;
	void *context;

    struct timespec period;  /**< Thread's period */
    struct timespec delay;   /**< Thread's startup delay */

} thrd_info_t;

/** Pool of real-time threads */
typedef struct {
    dlist_t *threads;        /**< List of thrd_t objects. (@see thrd.c) */
    size_t nthreads;         /**< Number of sampling threads. */

    uint8_t status;          /**< Status flags */
    int err;                 /**< Stores error codes. */
    int minprio;             /**< Minimum priority */
} thrd_pool_t;

typedef enum {
    THRD_ERR_LIBRARY  = 1 << 0,     /**< Library error */
    THRD_ERR_CLOSED   = 1 << 1,     /**< Added thread on a running pool */
    THRD_ERR_NULLPER  = 1 << 2      /**< Declared null period */
} thrd_err_t;

/** Initialize the pool
 *
 * @param pool The pool to be initialized;
 * @param minprio A positive priority offset over
 *                sched_get_priority_min()
 */
void thrd_init (thrd_pool_t *pool, unsigned minprio);

/** Destroy the pool
 *
 * This function is blocking: the pool will join all its threads before
 * freeing memory.
 *
 * @pool The pool to be freed.
 */
void thrd_destroy (thrd_pool_t *pool);

/** Add a new thread to the pool
 *
 * @see thrd_info_t.
 *
 * @param pool The pool to be extended with a new thread;
 * @param new_thrd The specification for the new thread.
 * @return 0 on success, -1 if the pool has been already started once by
 *         calling thrd_start.
 */
int thrd_add (thrd_pool_t *pool, const thrd_info_t * new_thrd);

/** Start the threads */
int thrd_start (thrd_pool_t *pool);

/** Get information on the error.
 *
 * After this call the internal error-keeping structure of the pool gets
 * resetted.
 *
 * @see thrd_err_t
 * @see thrd_strerr
 *
 * @param pool The pool for which the last call returned error;
 * @return The error.
 */
thrd_err_t   thrd_interr (thrd_pool_t *pool);

/** Get error description string.
 *
 * @param err The error;
 * @return The internal error description for the errors, strerror(3)
 *         return value when err is THRD_ERR_LIBRARY.
 */
const char * thrd_strerr (thrd_pool_t *pool, thrd_err_t err);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_thrd_h

