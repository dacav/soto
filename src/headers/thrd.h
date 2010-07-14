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

/** User definition for the thread. */
typedef struct {
    
    /** Callback of the thread
     *
     * @param context The specified user data;
     * @return zero in order to kill the thread; non-zero otherwise.
     */
    callback_t callback;

    /** User defined context for the callback */
    void *context;

    /** The user chan choose how to set the priority value.
     *
     * Basing on this flag we have a different interpretation for
     * thrd_info_t::priority.
     *
     * If thrd_info_t::prio_type is THRD_PRIO_EXPL then the provided
     * thread assignment will be used.
     *
     * If thrd_info_t::prio_type is THRD_PRIO_RM instead, the provided
     * thread will be computed basing on the period, which will be
     * compared with other THRD_PRIO_RM threads.
     *
     */
    enum {
        THRD_PRIO_RM,           /**< Priority assigned as Rate Monotonic. */
        THRD_PRIO_EXPL          /**< Explicitly setted priority. */
    } prio_type;

    int priority;               /**< Thread's priority */
    struct timespec period;     /**< Thread's period */
    struct timespec delay;      /**< Thread's startup delay */

} thrd_info_t;

typedef struct {
    pthread_t handler;          /**< Handler of the thread. */
    uint8_t status;             /**< Status flags. */
    thrd_info_t info;           /**< User defined thread info. */
} thrd_t; 

typedef struct {
    dlist_t *threads;           /**< List of thrd_t objects. */
    size_t nthreads;            /**< Number of sampling threads. */

    uint8_t status;             /**< Status flags */
    int err;                    /**< Stores error codes. */
    int minprio;                /**< Minimum priority */
} thrd_pool_t;

void thrd_init (thrd_pool_t *pool, int minprio);
void thrd_destroy (thrd_pool_t *pool);

int thrd_add (thrd_pool_t *pool, thrd_info_t * new_thrd);
int thrd_start (thrd_pool_t *pool);
void thrd_stop (thrd_pool_t *pool);

const char * thrd_strerr (thrd_pool_t *pool);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_thrd_h

