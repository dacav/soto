#include <stdio.h>
#include <stdlib.h>
#include <dacav/dacav.h>

#include "headers/options.h"

int main (int argc, char ** argv)
{
    opts_t opts;
    diter_t *i;

    if (opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    printf("dev=%s\nmode=%d\nrate=%u\nformat=%d\nprio=%d\n", opts.device,
            (int) opts.mode, opts.rate, opts.format, opts.minprio);

    i = dlist_iter_new(&opts.threads);
    while (diter_hasnext(i)) {
        thrinfo_t *info;

        info = diter_next(i);
        printf("Period %llu\n", (long long) info->period);
    }

    exit(EXIT_SUCCESS);
}
