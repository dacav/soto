#include <stdio.h>
#include <stdlib.h>

#include "headers/options.h"

int main (int argc, char ** argv)
{
    soto_opts_t opts;

    if (soto_opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    printf("dev=%s\nmode=%d\nrate=%u\nformat=%d\n", opts.device,
            (int) opts.mode, opts.rate, opts.format);

    exit(EXIT_SUCCESS);
}
