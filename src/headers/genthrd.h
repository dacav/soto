#ifndef __defined_headers_genthrd_h
#define __defined_headers_genthrd_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/thrd.h"

/** Opaque type for generic thread handle. */
typedef struct genth_data genth_t;

/** Subscribe a generic thread to the given pool.
 *
 * @param handle Thea address of a pointer where the generic thread
 *               handle address will be stored;
 * @param pool The pool to which the thread will be subscribed;
 * @param info Specification of the thread.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
int genth_subscribe (genth_t **handle, thrd_pool_t *pool,
                     const thrd_info_t *info);

/** Request thread termination.
 *
 * This call terminates the running thread.
 *
 * @param handle The handle of the thread.
 *
 * @retval 0 on success;
 * @retval -1 on failure (thread not started).
 */
int genth_sendkill (genth_t *handle);

/** Getter for the user context of the thread
 *
 * @param handle The handle of the thread.
 *
 * @return The internal context of the thread.
 */
void * genth_get_context (const genth_t *handle);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_genthrd_h

