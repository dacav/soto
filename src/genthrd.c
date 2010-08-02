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

#include <stdint.h>

#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/genthrd.h"
#include "headers/thrd.h"

struct genth_data {

    /* This objects have the same semantics as in thrd_info_t structure.
     * Since this module operates a sort of wrapper, here I keep the
     * wrapped stuff. */
    callback_t init;        
    callback_t callback;
    callback_t destroy;
	void *context;

    thrd_info_t user;

    struct {
        int active;             /* 1 if the thread is running; */
        pthread_t self;         /* Handler used for termination; */
    } thread;

};

static
int init_cb (void *arg)
{
    struct genth_data *ctx = (struct genth_data *)arg;
    thrd_info_t *user;

    user = &ctx->user;
    if (user->init != NULL && user->init(user->context)) {
        return 1;
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    ctx->thread.active = 1;
    ctx->thread.self = pthread_self();
    return 0;
}

static
void destroy_cb (void *arg)
{
    struct genth_data *ctx = (struct genth_data *)arg;

    if (ctx->destroy) {
        ctx->destroy(ctx->context);
    }
    free(arg);
}

static
int thread_cb (void *arg)
{
    struct genth_data *ctx = (struct genth_data *)arg;

    if (ctx->callback(ctx->context)) {
        destroy_cb(arg);
        return 1;
    }

    /* Check cancellations. If it is the case the destroy_cb will manage
     * memory deallocation. */
    pthread_cleanup_push(destroy_cb, arg);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_cleanup_pop(0);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    return 0;
}

int genth_subscribe (genth_t **handle, thrd_pool_t *pool,
                     const thrd_info_t *info)
{
    thrd_info_t thi;
    struct genth_data *ctx;
    int err;

    thi.init = init_cb;
    thi.callback = thread_cb;
    thi.destroy = NULL;
    rtutils_time_copy(&thi.delay, &info->delay);
    rtutils_time_copy(&thi.period, &info->period);

    ctx = (struct genth_data *) calloc(1, sizeof(struct genth_data));
    assert(ctx);
    thi.context = (void *) ctx;

    ctx->init = info->init;
    ctx->callback = info->callback;
    ctx->destroy = info->destroy;
    ctx->context = info->context;
    ctx->thread.active = 0;

    if ((err = thrd_add(pool, &thi)) != 0) {
        free(ctx);
        *handle = NULL;
    } else {
        *handle = ctx;
    }
    return err;
}

int genth_sendkill (genth_t *handle)
{
    if (handle == NULL) {
        return -1;
    }
    if (handle->thread.active) {
        int err;
        
        /* The cancellation will be checked explicitly in thread_cb().
         * Also note that the thread pool (see headers/thrd.h) is in
         * charge of joining the thread.
         */
        err = pthread_cancel(handle->thread.self);

        assert(!err);
        return 0;
    } else {
        return -1;
    }
}

void * genth_get_context (const genth_t *handle)
{
    return handle->context;
}
