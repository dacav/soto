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

/** @file dispatch.h
 *
 * This module provieds a dispatching system based on thdqueues. It
 * implements a thread which extracts data from an incoming thread queue,
 * builds copies of the item and sends a copy for each outgoing thread
 * queue.
 *
 */

#ifndef __defined_headers_dispatch_h
#define __defined_headers_dispatch_h
#ifdef __cplusplus
extern "C" {
#endif

#include <thdacav/thdacav.h>
#include <dacav/dacav.h>
#include <pthread.h>

/** Copy constructor callback. */
typedef void * (* disp_dup_t) (void *);

typedef struct {
    thdqueue_t *input;      /**< Queue of incoming objects; */
    dlist_t *outputs;       /**< List of quques for outgoing objects; */
    size_t noutputs;        /**< Number of output queues; */
    disp_dup_t dup;         /**< Copy constructor for the dispatched
                             *   object; */

    pthread_mutex_t lock;   /**< Protect the outputs queue. */
    pthread_t thread;       /**< The dispatching thread */

    int active;             /**< Flag preventing new hooks creation if the
                             *   producer already finished */
} disp_t;

/** Constructor for the dispatcher.
 *
 * @param disp The dispatcher to be built;
 * @param input The input queue of the dispatcher;
 * @param dup A copy constructor for the dispatched object. If NULL is
 *            provided all output queues shall be fed with the same
 *            instance.
 */
void disp_init (disp_t *disp, thdqueue_t *input, disp_dup_t dup);

/** Getter for a new output queue.
 *
 * This function allocated in a thread-safe manner a new output queue. The 
 *
 * @param disp The dispatcher.
 *
 * @return The new queue or NULL if the 
 */
thdqueue_t * disp_new_hook (disp_t *disp);

/** Destructor for the dispatcher.
 *
 * @param disp The dispatcher to be freed.
 */
void disp_destroy (disp_t *disp);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_dispatch_h

