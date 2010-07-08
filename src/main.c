#include <stdio.h>
#include <stdlib.h>
#include <dacav/dacav.h>
#include <unistd.h>
#include <alloca.h>

#include <sched.h>

#include "headers/options.h"
#include "headers/thrd.h"

int main (int argc, char ** argv)
{
    opts_t *opts;
    thrd_pool_t *pool;

    opts = alloca(sizeof(opts_t));
    pool = alloca(sizeof(thrd_pool_t));

    if (opts_parse(opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    if (thrd_init(pool, opts) != 0) {
        fprintf(stderr, "EPIC FAILURE %s\n", thrd_strerr(pool));
        exit(EXIT_FAILURE);
    }

    sleep(1);

    DEBUG_FMT("Ready??... go!%c", 10);
    thrd_enable_switch(pool);

    pause();

    thrd_destroy(pool);

    exit(EXIT_SUCCESS);
}
