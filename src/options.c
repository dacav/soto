/*
 * Copyright 2010 Giovanni Simoni
 *
 * This file is part of Soto.
 *
 * Soto is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Soto is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Soto.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "headers/options.h"
#include "headers/config.h"
#include "headers/rtutils.h"
#include "headers/constants.h"

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sched.h>
#include <assert.h>
#include <stdbool.h>

struct opts {

    const char *device;         /**< PCM device */
    unsigned rate;              /**< Sample rate; */
    
    /* Minimum priority value to be used. This will be added to the
     * result of the sched_get_priority_min() syscall.
     */
    int minprio;

    /* What shall be shown */
    enum {
        SHOW_NOTHING  = 0,
        SHOW_SIGNAL   = 1 << 0,
        SHOW_SPECTRUM = 1 << 1,
        SHOW_BOTH     = 1 | (1 << 1)
    } show;

    /* Multiplicative factor that defines the proportion between the
     * alsa-defined sampling buffer size and the buffer where the samples
     * shall be stored */
    unsigned buffer_scale;

    unsigned run_for;
};

static const char optstring[] = "d:r:m:U::u::s:t:h";
static const struct option longopts[] = {
    {"dev", 1, NULL, 'd'},
    {"rate", 1, NULL, 'r'},
    {"minprio", 1, NULL, 'm'},
    {"show-spectrum", 2, NULL, 'U'},
    {"show-signal", 2, NULL, 'u'},
    {"buffer-scale", 1, NULL, 's'},
    {"run-for", 1, NULL, 't'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

static const char help [] =
"\n" PACKAGE_STRING "\n"
"Usage: %s [options]\n\n"
"  --dev={dev} | -d {dev}\n"
"        Specify an audio device (default: \"" DEFAULT_DEVICE "\");\n\n"
"  --rate={rate} | -r {rate}\n"
"        Specify a sample rate for ALSA in Hertz (default: 44100);\n\n"
"  --show-spectrum[={bool}] | -U [{bool}]\n"
"        Show the spectrum of the audio stream (default: yes);\n\n"
"  --show-signal[={bool}] | -u [{bool}] \n"
"        Show the signal of the audio stream (default: no);\n\n"
"  --buffer-scale={factor} | -s {factor}\n"
"        Provide the proportion between sampling buffer and read buffer\n"
"        (default: 10);\n\n"
"  --minprio={priority} | -m {priority}\n"
"        Specify the realtime priority for the thread having the longest\n"
"        sampling period (default 0, required a positive integer);\n\n"
"  --run-for={time in seconds} | -r {time in seconds}\n"
"        Requires the program to run for a certain amount of time.\n"
"        By providing 0 (which is the default) the program will run\n"
"        until interrupted\n\n"
"  --help  | -h\n"
"        Print this help.\n";

static inline
void print_help (const char *progname)
{
    fprintf(stderr, help, progname);
}

static inline
void notify_error (const char *progname, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ", progname);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
    print_help(progname);
}

static
int to_unsigned (const char *arg, unsigned *val)
{
    return sscanf(arg, "%u", val) == 1 ? 0 : -1;
}

static 
int check_case_optarg (const char *arg, const char *allw[])
{
    int id = 0;

    while (allw[id]) {
        if (strcasecmp(allw[id], arg) == 0) {
            return id;
        }
        id ++;
    }
    return -1;
}

static
int to_bool (const char *arg, bool *val)
{
    const char *allowed_true[] = {
        "yes", "y", "true", "t", "1", NULL
    };
    const char *allowed_false[] = {
        "no", "n", "false", "f", "0", NULL
    };

    if (arg == NULL || check_case_optarg(arg, allowed_true) >= 0) {
        *val = true;
        return 0;
    }

    if (check_case_optarg(arg, allowed_false) >= 0) {
        *val = false;
        return 0;
    }

    return -1;
}

static
int to_priority (const char *arg, int *prio)
{
    int val;

    if (to_unsigned(arg, (unsigned *)&val) == -1)
        return -1;
    if (val + sched_get_priority_min(SCHED_FIFO) >
            sched_get_priority_max(SCHED_FIFO))
        return -2;
    *prio = val;
    return 0;
}

static
void set_defaults (opts_t *so)
{
    so->device = DEFAULT_DEVICE;
    so->rate = DEFAULT_RATE;
    so->minprio = DEFAULT_MINPRIO;
    so->show = SHOW_SPECTRUM;
    so->buffer_scale = DEFAULT_BUFFER_SCALE;
    so->run_for = DEFAULT_RUN_FOR;
}

opts_t * opts_parse (int argc, char * const argv[])
{
    extern char *optarg;
    int opt;
    bool b;

    opts_t *so = calloc(1, sizeof(opts_t));

    set_defaults(so);
    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL))
           != -1) {
        switch (opt) {
            case 'd':
                so->device = optarg;
                break;
            case 'r':
                if (to_unsigned(optarg, &so->rate)) {
                    notify_error(argv[0], "invalid rate: '%s'", optarg);
                    return NULL;
                }
                break;
            case 'U':
                if (to_bool(optarg, &b)) {
                    notify_error(argv[0], "cannot evaluate '%s' as bool",
                                 optarg);
                    return NULL;
                }
                if (b) {
                    so->show |= SHOW_SPECTRUM;
                } else {
                    so->show &= ~SHOW_SPECTRUM;
                }
                break;
            case 'u':
                if (to_bool(optarg, &b)) {
                    notify_error(argv[0], "cannot evaluate '%s' as bool",
                                 optarg);
                    return NULL;
                }
                if (b) {
                    so->show |= SHOW_SIGNAL;
                } else {
                    so->show &= ~SHOW_SIGNAL;
                }
                break;
            case 's':
                if (to_unsigned(optarg, &so->buffer_scale)) {
                    notify_error(argv[0], "invalid scale: '%s'", optarg);
                    return NULL;
                }
                break;
            case 'm':
                switch (to_priority(optarg, &so->minprio)) {
                    case -1:
                        notify_error(argv[0],
                                     "invalid min priority value: '%s'",
                                     optarg);
                        return NULL;
                    case -2:
                        notify_error(argv[0], "min priority too high: %s",
                                     optarg);
                        return NULL;
                }
                break;
            case 't':
                if (to_unsigned(optarg, &so->run_for)) {
                    notify_error(argv[0], "invalid time of execution: '%s'", optarg);
                    return NULL;
                }
                break;
            case 'h':
            case '?':
                print_help(argv[0]);
                return NULL;
        }
    }
    if (so->show == SHOW_NOTHING) {
        notify_error(argv[0], "What should I plot?");
        return NULL;
    }
    return so;
}

/* Funny regex generated getters:
   %s/\v^(.*)/opts_get_\1 (opts_t *o)\r{\r    return o->\1;\r}\r
 */

const char * opts_get_device (opts_t *o)
{
    return o->device;
}

unsigned opts_get_rate (opts_t *o)
{
    return o->rate;
}

unsigned opts_get_minprio (opts_t *o)
{
    return o->minprio;
}

unsigned opts_get_buffer_scale (opts_t *o)
{
    return o->buffer_scale;
}

bool opts_spectrum_shown (opts_t *o)
{
    return (o->show & SHOW_SPECTRUM) != 0;
}

bool opts_signal_shown (opts_t *o)
{
    return (o->show & SHOW_SIGNAL) != 0;
}

unsigned opts_get_run_for (opts_t *o)
{
    return o->run_for;    
}
