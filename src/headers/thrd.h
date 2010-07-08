#ifndef __defined_headers_thrd_h
#define __defined_headers_thrd_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <dacav/dacav.h>

#include "headers/options.h"
#include "headers/logging.h"

typedef struct {
    pthread_mutex_t mux;        /**< Sync mutex */
    pthread_cond_t cond;
    volatile uint8_t * ctrl;
} thrd_sync_t;

typedef struct {
    pthread_t handler;          /**< Handler of the thread. */

    thrd_sync_t *sync;          /**< Backlink to synchronization
                                 *   structure */

    uint64_t period;            /**< Activation period. */
    int priority;               /**< Thread's priority */

    uint8_t status;             /**< Status flags. */
} thrd_t; 

typedef struct {
    thrd_t ** threads;          /**< Sampling threads data. */
    size_t nthreads;            /**< Number of sampling threads. */

    thrd_sync_t sync;
    uint8_t status;             /**< Status flags */
    int err;                    /**< Stores error codes. */
} thrd_pool_t;

int thrd_init (thrd_pool_t *pool, opts_t *opts);
void thrd_destroy (thrd_pool_t *pool);

void thrd_enable_switch (thrd_pool_t *pool);

const char * thrd_strerr (thrd_pool_t *pool);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_thrd_h

