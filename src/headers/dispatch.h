#ifndef __defined_headers_dispatch_h
#define __defined_headers_dispatch_h
#ifdef __cplusplus
extern "C" {
#endif

#include <thdacav/thdacav.h>
#include <dacav/dacav.h>
#include <pthread.h>

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

