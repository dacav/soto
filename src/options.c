#include "headers/options.h"
#include "headers/config.h"

#include <stdio.h>
#include <getopt.h>
#include <string.h>

static const char optstring[] = "d:h";
static const struct option longopts[] = {
    {"device", 1, NULL, 'd'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

const char help [] =
    PACKAGE_STRING "\n"
    "Usage: %s [options]\n\n"
    "Available options\n\n"
    "  --device=<dev> | -d <dev>\n"
    "        Specify an audio device\n\n"
    "  --help  | -h\n"
    "        Print this help\n";

static inline
void print_help (const char *progname)
{
    fprintf(stderr, help, progname);
}

int soto_opts_parse (soto_opts_t *soto_opts, int argc,
                     char * const argv[])
{
    int opt;

    /* Default values for options */
    soto_opts->device = "hw:0,0"

    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL))
           != -1) {
        switch (opt) {
            case 'd':
                soto_opts->device = optarg;
                break;
            case 'h':
                print_help(argv[0]);
                return -1;
        }
    }
    return 0;
}

