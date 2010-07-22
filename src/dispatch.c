#include <string.h>

#include "headers/dispatch.h"
#include "headers/constants.h"
#include "headers/logging.h"

static
void * phony_copy_constructor (void * x)
{
    return x;
}

void disp_init (disp_t *disp, thdqueue_t *input, disp_dup_t dup)
{
    disp->input = input;
    disp->outputs = dlist_new();
    disp->noutputs = 0;

    /* Here I use the identity function as phony constructor. This may
     * seem foolish ("why don't we simply check 'dup' instead of doing a
     * useless function call?") but watch at thread_cp and it will become
     * more elegant!
     */
    disp->dup = (dup == NULL ? phony_copy_constructor : dup);

    pthread_mutex_init(&disp->lock, NULL);
}

thdqueue_t * disp_new_hook (disp_t *disp)
{
    thdqueue_t *hook;
   
    pthread_mutex_lock(&disp->lock);
    hook = thdqueue_new();
    disp->outputs = dlist_append(disp->outputs, (void *)hook);
    disp->noutputs ++;
    pthread_mutex_unlock(&disp->lock);

    return hook;
}

void disp_destroy (disp_t *disp)
{
    pthread_mutex_destroy(&disp->lock);
    dlist_free(disp->outputs, (dfree_cb_t) thdqueue_free);
}

static
int thread_cb (void *arg)
{
    disp_t *disp = (disp_t *)arg;
    diter_t *iter;
    void *message;
    int ret = 0;

    pthread_mutex_lock(&disp->lock);
    iter = dlist_iter_new(&disp->outputs);
    if (thdqueue_extract(disp->input, &message) == THDQUEUE_ENDDATA) {
        while (diter_hasnext(iter)) {
            /* Propagate ENDDATA trough all output queues. */
            thdqueue_enddata((thdqueue_t *) diter_next(iter));
        }
        ret = 1;
    } else {
        disp_dup_t dup = disp->dup;
        void *copy;
        unsigned cnt = 1;

        while (diter_hasnext(iter)) {
            /* Propagate readed data trough all output queues.
             * Note: Unless the user provided NULL as dup, each output
             *       queue is fed with a private instance of the data.
             */
            if (cnt == disp->noutputs) {
                /* Also the original object must be freed somehow. Since
                 * we don't know anything about what object it is, we
                 * simply avoid duplicates on the last queue and send
                 * directly the original one.
                 */
                copy = message;
            } else {
                copy = dup(message);
            }
            if (thdqueue_insert((thdqueue_t *) diter_next(iter), message)
                    == THDQUEUE_UNALLOWED) {
                DEBUG_MSG("Dispatcher's output channels closed? "
                          "Impossible.");
                abort();
            }
            cnt ++;
        }
    }
    dlist_iter_free(iter);
    pthread_mutex_unlock(&disp->lock);

    return ret;
}

int disp_subscribe (disp_t *disp, thrd_pool_t *pool,
                    const struct timespec *period)
{
    thrd_info_t info = {
        .init = NULL,
        .callback = thread_cb,
        .destroy = NULL,
        .context = (void *) disp
    };

    memcpy((void *)&info.period, (const void *)period,
           sizeof(struct timespec));
    info.delay.tv_sec = STARTUP_DELAY_SEC;
    info.delay.tv_nsec = STARTUP_DELAY_nSEC;

    return thrd_add(pool, &info);
}
