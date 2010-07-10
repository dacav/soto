#include <stdio.h>
#include <stdlib.h>
#include <dacav/dacav.h>
#include <unistd.h>
#include <alloca.h>
#include <sched.h>

#include "headers/options.h"
#include "headers/thrd.h"

static
int f (opts_thrd_t *o)
{
    uint64_t n = o->period;
    while (n--) {
        DEBUG_FMT("Countdown %d", (int)n);
    }
    DEBUG_MSG("End of the story");

    return 0;
}

static
dlist_t * build_thread_list (dlist_t *opts_thrd, int minprio)
{
    diter_t *i;
    dlist_t *ret = dlist_new();

    i = dlist_iter_new(&opts_thrd);
    while (diter_hasnext(i)) {
        thrd_info_t *info;

        assert(info = malloc(sizeof(thrd_info_t)));
        info->priority = minprio ++;
        info->context = diter_next(i);
        info->callback = (int (*) (void *))f;
        ret = dlist_append(ret, (void *)info);
    }
    dlist_iter_free(i);

    return ret;
}

int main (int argc, char ** argv)
{
    opts_t opts;
    thrd_pool_t pool;
    dlist_t *threads;

    if (opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    threads = build_thread_list(opts.threads, opts.minprio);

    DEBUG_MSG("Start");
    if (thrd_init(&pool, threads, opts.nthreads) == -1) {
        LOG_FMT("Shitstorm happened: %s", thrd_strerr(&pool));
        exit(EXIT_FAILURE);
    }
    DEBUG_MSG("Everything ready... now sleeping for a while");

    sleep(1);

    DEBUG_MSG("GO!");
    thrd_start(&pool);

    sleep(3);
    //sched_yield();

    thrd_destroy(&pool);
    dlist_free(threads, free);
    opts_destroy(&opts);

    exit(EXIT_SUCCESS);
}
