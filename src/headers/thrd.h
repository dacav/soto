#ifndef __defined_headers_thrd_h
#define __defined_headers_thrd_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <dacav/dacav.h>

#include "headers/logging.h"

typedef struct {
    pthread_mutex_t mux;        /**< Sync mutex */
    pthread_cond_t cond;
    volatile uint8_t * ctrl;
} thrd_sync_t;

/** User definition for the thread. */
typedef struct {
    
    /** Callback of the thread
     *
     * @param context The specified user data;
     * @return zero in order to kill the thread; non-zero otherwise.
     */
    int (* callback) (void *context);

    void *context;              /**< User defined context for the callback */
    int priority;               /**< Thread's priority */

} thrd_info_t;

typedef struct {
    pthread_t handler;          /**< Handler of the thread. */
    thrd_sync_t *sync;          /**< Backlink to synchronization
                                 *   structure. */
    uint8_t status;             /**< Status flags. */
    thrd_info_t *info;          /**< User defined thread info. */
} thrd_t; 

typedef struct {
    thrd_t * threads;           /**< Sampling threads data. */
    size_t nthreads;            /**< Number of sampling threads. */

    thrd_sync_t sync;
    uint8_t status;             /**< Status flags */
    int err;                    /**< Stores error codes. */
} thrd_pool_t;

int thrd_init (thrd_pool_t *pool, dlist_t *threads, size_t nthreads);

void thrd_destroy (thrd_pool_t *pool);

void thrd_start (thrd_pool_t *pool);

const char * thrd_strerr (thrd_pool_t *pool);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_thrd_h

