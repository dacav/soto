#include <stdio.h>
#include <stdlib.h>
#include <dacav/dacav.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>

#include "headers/options.h"
#include "headers/thrd.h"

int main (int argc, char ** argv)
{
    thrd_pool_t pool;
    thrd_info_t info;

    thrd_init(&pool, 17);

    info.period.tv_nsec = 0ULL;
    info.prio_type = THRD_PRIO_RM;
    info.delay.tv_nsec = 0ULL;
    info.delay.tv_sec = 2ULL;

    info.period.tv_sec = 3;
    assert(thrd_add(&pool, &info) == 0);
    info.period.tv_sec = 1;
    assert(thrd_add(&pool, &info) == 0);
    info.period.tv_sec = 5;
    assert(thrd_add(&pool, &info) == 0);

    info.prio_type = THRD_PRIO_EXPL;
    info.period.tv_sec = 0L;

    info.priority = 12;
    assert(thrd_add(&pool, &info) == 0);
    info.priority = 18;
    assert(thrd_add(&pool, &info) == 0);
    info.priority = 20;
    assert(thrd_add(&pool, &info) == 0);

    if (thrd_start(&pool)) {
        DEBUG_FMT("%s", thrd_strerr(&pool));
    }

    thrd_destroy(&pool);
}
