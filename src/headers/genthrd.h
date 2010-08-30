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

/** @file genthrd.h */

/** @addtogroup GenThrd */
/*@{*/

#ifndef __defined_headers_genthrd_h
#define __defined_headers_genthrd_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/thrd.h"

/** @brief Opaque type for generic thread handle. */
typedef struct genth_data genth_t;

/** @brief Subscribe a generic thread to the given pool.
 *
 * @param handle Thea address of a pointer where the generic thread
 *               handle address will be stored;
 * @param pool The pool to which the thread will be subscribed;
 * @param info Specification of the thread.
 *
 * @see headers/thrd.h.
 *
 * @return This function just adds something to pool, therefore you may
 *         interpret its return value as if it were thrd_add().
 */
const thrd_rtstats_t * genth_subscribe (genth_t **handle,
                                        thrd_pool_t *pool,
                                        const thrd_info_t *info);

/** @brief Request thread termination.
 *
 * This call terminates the running thread.
 *
 * @param handle The handle of the thread.
 *
 * @retval 0 on success;
 * @retval -1 on failure (thread not started).
 */
int genth_sendkill (genth_t *handle);

/** @brief Getter for the user context of the thread
 *
 * @param handle The handle of the thread.
 *
 * @return The internal context of the thread.
 */
void * genth_get_context (const genth_t *handle);

/*@}*/

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_genthrd_h

