#include <string.h>
#include <assert.h>

#include "headers/dispatch.h"
#include "headers/constants.h"
#include "headers/logging.h"

static
void * phony_copy_constructor (void * x)
{
    return x;
}

static
void * thread_cb (void *arg)
{
    disp_t *disp = (disp_t *)arg;
    disp_dup_t dup = disp->dup;
    diter_t *iter;
    void *message;

    while (thdqueue_extract(disp->input, &message) != THDQUEUE_ENDDATA) {
        void *copy;
        unsigned cnt = 1;

        pthread_mutex_lock(&disp->lock);
        iter = dlist_iter_new(&disp->outputs);
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
            if (thdqueue_insert((thdqueue_t *) diter_next(iter), copy)
                    == THDQUEUE_UNALLOWED) {
                DEBUG_MSG("Dispatcher's output channels closed? "
                          "Impossible.");
                abort();
            }
            cnt ++;
        }
        dlist_iter_free(iter);
        pthread_mutex_unlock(&disp->lock);
    }

    /* Propagate ENDDATA trough all output queues. */
    pthread_mutex_lock(&disp->lock);
    disp->active = 0;
    iter = dlist_iter_new(&disp->outputs);
    while (diter_hasnext(iter)) {
        thdqueue_enddata((thdqueue_t *) diter_next(iter));
    }
    dlist_iter_free(iter);
    pthread_exit(EXIT_SUCCESS);
}

void disp_init (disp_t *disp, thdqueue_t *input, disp_dup_t dup)
{
    int err;

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
    disp->active = 1;
    err = pthread_create(&disp->thread, NULL, thread_cb, (void *)disp);
    assert(err == 0);
}

thdqueue_t * disp_new_hook (disp_t *disp)
{
    thdqueue_t *hook = NULL;
   
    pthread_mutex_lock(&disp->lock);
    if (disp->active) {
        hook = thdqueue_new();
        disp->outputs = dlist_append(disp->outputs, (void *)hook);
        disp->noutputs ++;
    }
    pthread_mutex_unlock(&disp->lock);

    return hook;
}

void disp_destroy (disp_t *disp)
{
    pthread_join(disp->thread, NULL);
    pthread_mutex_destroy(&disp->lock);
    dlist_free(disp->outputs, (dfree_cb_t) thdqueue_free);
}

