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

/** Callback of the thread
*
* @param context The specified user data;
* @return zero in order to keep the thread running. Any other value
*         stops the periodic execution.
*/
typedef int (* callback_t) (void *context);

/** User definition for the thread.
 *
 * This is used as argument for the thrd_init function.
 */
typedef struct {

    /** Called by the thread before starting cycling periodically. You may
     * specify it as NULL.
     *
     * @param context The specified user data;
     * @return zero in order to let the execution start. Any other value
     *         aborts the execution.
     */
    callback_t init;        

    /** Business logic of the thread. You must provide it.
     *
     * @param context The specified user data;
     * @return zero in order to keep the thread running. Any other value
     *         stops the periodic execution.
     */
    callback_t callback;
    
    /** Called by the thread before termination. You may specify it as
     * NULL. 
     *
     * @param context The specified user data;
     * @return Ignored.
     */
    callback_t destroy;

    /** Context of thrd_info_t::init, thrd_info_t::callback and
     *  thrd_info_t::final
     */
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
    THRD_ERR_NULLPER  = 1 << 2,     /**< Declared null period */
    THRD_ERR_EMPTY    = 1 << 3      /**< No thread subscribed */
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

/** Start the threads
 *
 * This call enables the thread. Before calling it you must add at least
 * one thread the pool. The call needs the appropriate level of privilege,
 * otherwise it shall fail miserably.
 *
 * @see thrd_interr
 * @see thrd_strerr
 *
 * @param pool The pool to be started;
 * @return 0 on success, not 0 on error.
 */
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
thrd_err_t thrd_interr (thrd_pool_t *pool);

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

