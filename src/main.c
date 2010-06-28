#include <stdio.h>
#include <stdlib.h>

#include "headers/options.h"

int main (int argc, char ** argv)
{
    soto_opts_t opts;

    if (soto_opts_parse(&opts, argc, argv)) {
        exit(EXIT_FAILURE);
    }

    printf("LOL %s\n", opts.device);

    exit(EXIT_SUCCESS);
}
