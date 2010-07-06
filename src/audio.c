#include "headers/audio.h"
#include "headers/options.h"

#include <sched.h>

audio_err_t audio_init (opts_t *opts)
{
    struct sched_param param = {
        .sched_priority = opts->minprio;
    };

    sched_setparam(0, SCHED_FIFO, &param);
}
