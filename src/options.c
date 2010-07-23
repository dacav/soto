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

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sched.h>
#include <assert.h>

/* TODO Parametrize default values for options through `configure.ac'
 *      Produce a possibly verbose output
 */
#define DEFAULT_RATE            44100
#define DEFAULT_DEVICE          "hw:0,0"
#define DEFAULT_MINPRIO         0
#define DEFAULT_PERIOD_SLOTS    10

static const char optstring[] = "d:r:m:p:B:c:h";
static const struct option longopts[] = {
    {"alsa-dev", 1, NULL, 'd'},
    {"alsa-rate", 1, NULL, 'r'},      // rate 
    {"alsa-mode", 1, NULL, 'm'},      // mono, stereo
    {"min-prio", 1, NULL, 'p'},
    {"sample-buffer", 1, NULL, 'B'},
    {"constraint", 1, NULL, 'c'},
    {"help", 0, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

static const char *avail_mode[] = {
    "mono", "stereo", NULL
};

static const char *avail_policy[] = {
    "fixrate", "fixperiod"
};

static const char help [] =
"\n" PACKAGE_STRING "\n"
"Usage: %s [options]\n\n"
"  --alsa-dev={dev} | -d {dev}\n"
"        Specify an audio device (default: \"hw:0,0\");\n\n"
"  --alsa-rate={rate} | -r {rate}\n"
"        Specify a sample rate for ALSA in Hertz (default: 44100);\n\n"
"  --alsa-mode={mode} | -m {mode}\n"
"        Specify a sample mode (default: STEREO, also MONO is\n"
"        available);\n\n"
"  --sample-buffer={count} | -B {count}\n"
"        Specify the number of samples (according to the '--alsa-rate'\n"
"        parameter) to be buffered\n\n"
"  --constraint={ fixrate | fixperiod } | -c { fixrate | fixperiod }\n"
"        Require the program to fail if the given '--alsa-rate' and\n"
"        '--sampler-period' options cannot be honored;\n\n"
"  --min-prio={priority}\n"
"        Specify the realtime priority for the thread having the longest\n"
"        sampling period (default 0, required a positive integer);\n\n"
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
int check_optarg (const char *arg, const char *allw[])
{
    int id = 0;

    while (allw[id]) {
        if (strcmp(allw[id], arg) == 0) {
            return id;
        }
        id ++;
    }
    return -1;
}

static
int to_unsigned (const char *arg, unsigned *val)
{
    return sscanf(arg, "%u", val) == 1 ? 0 : -1;
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
    so->mode = STEREO;
    so->rate = DEFAULT_RATE;
    so->minprio = DEFAULT_MINPRIO;
    so->nsamp = DEFAULT_PERIOD_SLOTS;
    so->policy = SAMP_ACCEPT_RATE;
}

#if 0
static
int to_thrdinfo (const char *arg, opts_thrd_t *thr)
{
    uint64_t p;

    if (sscanf(arg, "%llu", (long long *) &p) != 1)
        return -1;
    if (p == 0)
        return -1;

    thr->period = p;
    return 0;
}
#endif

extern char *optarg;

int opts_parse (opts_t *so, int argc, char * const argv[])
{
    int opt;

    set_defaults(so);
    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL))
           != -1) {
        switch (opt) {
            case 'd':
                so->device = optarg;
                break;
            case 'm':
                switch (check_optarg(optarg, avail_mode)) {
                    case 0:
                        so->mode = MONO;
                        break;
                    case 1:
                        so->mode = STEREO;
                        break;
                    default:
                        notify_error(argv[0], "invalid mode: '%s'", optarg);
                        return -1;
                }
                break;
            case 'r':
                if (to_unsigned(optarg, &so->rate) == -1) {
                    notify_error(argv[0], "invalid rate: '%s'", optarg);
                    return -1;
                }
                break;
            case 'B':
                if (to_unsigned(optarg, &so->nsamp) == -1) {
                    notify_error(argv[0], "invalid buffered samples count: '%s'", optarg);
                    return -1;
                }
                break;
            case 'p':
                switch (to_priority(optarg, &so->minprio)) {
                    case -1:
                        notify_error(argv[0],
                                     "invalid min priority value: '%s'",
                                     optarg);
                        return -1;
                    case -2:
                        notify_error(argv[0], "min priority too high: %s",
                                     optarg);
                        return -1;
                }
                break;
            case 'c':
                if (check_optarg(optarg, avail_policy) == 0) {
                        printf("not accepting rate\n");
                        so->policy &= ~SAMP_ACCEPT_RATE;
                }
                break;
            case 'h':
            case '?':
                print_help(argv[0]);
                return -1;
        }
    }
    return 0;
}

/*

   The remaining part of this file contains useful data for a possible
   future expansion of the program, namely the valid options for the input
   audio format. Only S16_LE is be supported for the moment.

   Inb4: Yes, they have been extrapolated by regexes.

 */

#if 0

static const char *avail_format[] = {
    "UNKNOWN", "S8", "U8", "S16_LE", "S16_BE", "U16_LE", "U16_BE",
    "S24_LE", "S24_BE", "U24_LE", "U24_BE", "S32_LE", "S32_BE", "U32_LE",
    "U32_BE", "FLOAT_LE", "FLOAT_BE", "FLOAT64_LE", "FLOAT64_BE",
    "IEC958_SUBFRAME_LE", "IEC958_SUBFRAME_BE", "MU_LAW", "A_LAW",
    "IMA_ADPCM", "MPEG", "GSM", "SPECIAL", "S24_3LE", "S24_3BE",
    "U24_3LE", "U24_3BE", "S20_3LE", "S20_3BE", "U20_3LE", "U20_3BE",
    "S18_3LE", "S18_3BE", "U18_3LE", "U18_3BE", "S16", "U16", "S24",
    "U24", "S32", "U32", "FLOAT", "FLOAT64", "IEC958_SUBFRAME", NULL
};

static
int to_pcm_format (int index, snd_pcm_format_t *fmt)
{
    switch (index) {
        case  0: *fmt = SND_PCM_FORMAT_UNKNOWN;              break;
        case  1: *fmt = SND_PCM_FORMAT_S8;                   break;
        case  2: *fmt = SND_PCM_FORMAT_U8;                   break;
        case  3: *fmt = SND_PCM_FORMAT_S16_LE;               break;
        case  4: *fmt = SND_PCM_FORMAT_S16_BE;               break;
        case  5: *fmt = SND_PCM_FORMAT_U16_LE;               break;
        case  6: *fmt = SND_PCM_FORMAT_U16_BE;               break;
        case  7: *fmt = SND_PCM_FORMAT_S24_LE;               break;
        case  8: *fmt = SND_PCM_FORMAT_S24_BE;               break;
        case  9: *fmt = SND_PCM_FORMAT_U24_LE;               break;
        case 10: *fmt = SND_PCM_FORMAT_U24_BE;               break;
        case 11: *fmt = SND_PCM_FORMAT_S32_LE;               break;
        case 12: *fmt = SND_PCM_FORMAT_S32_BE;               break;
        case 13: *fmt = SND_PCM_FORMAT_U32_LE;               break;
        case 14: *fmt = SND_PCM_FORMAT_U32_BE;               break;
        case 15: *fmt = SND_PCM_FORMAT_FLOAT_LE;             break;
        case 16: *fmt = SND_PCM_FORMAT_FLOAT_BE;             break;
        case 17: *fmt = SND_PCM_FORMAT_FLOAT64_LE;           break;
        case 18: *fmt = SND_PCM_FORMAT_FLOAT64_BE;           break;
        case 19: *fmt = SND_PCM_FORMAT_IEC958_SUBFRAME_LE;   break;
        case 21: *fmt = SND_PCM_FORMAT_IEC958_SUBFRAME_BE;   break;
        case 22: *fmt = SND_PCM_FORMAT_MU_LAW;               break;
        case 23: *fmt = SND_PCM_FORMAT_A_LAW;                break;
        case 24: *fmt = SND_PCM_FORMAT_IMA_ADPCM;            break;
        case 25: *fmt = SND_PCM_FORMAT_MPEG;                 break;
        case 26: *fmt = SND_PCM_FORMAT_GSM;                  break;
        case 27: *fmt = SND_PCM_FORMAT_SPECIAL;              break;
        case 28: *fmt = SND_PCM_FORMAT_S24_3LE;              break;
        case 29: *fmt = SND_PCM_FORMAT_S24_3BE;              break;
        case 31: *fmt = SND_PCM_FORMAT_U24_3LE;              break;
        case 32: *fmt = SND_PCM_FORMAT_U24_3BE;              break;
        case 33: *fmt = SND_PCM_FORMAT_S20_3LE;              break;
        case 34: *fmt = SND_PCM_FORMAT_S20_3BE;              break;
        case 35: *fmt = SND_PCM_FORMAT_U20_3LE;              break;
        case 36: *fmt = SND_PCM_FORMAT_U20_3BE;              break;
        case 37: *fmt = SND_PCM_FORMAT_S18_3LE;              break;
        case 38: *fmt = SND_PCM_FORMAT_S18_3BE;              break;
        case 39: *fmt = SND_PCM_FORMAT_U18_3LE;              break;
        case 40: *fmt = SND_PCM_FORMAT_U18_3BE;              break;
        case 41: *fmt = SND_PCM_FORMAT_S16;                  break;
        case 42: *fmt = SND_PCM_FORMAT_U16;                  break;
        case 43: *fmt = SND_PCM_FORMAT_S24;                  break;
        case 44: *fmt = SND_PCM_FORMAT_U24;                  break;
        case 45: *fmt = SND_PCM_FORMAT_S32;                  break;
        case 46: *fmt = SND_PCM_FORMAT_U32;                  break;
        case 47: *fmt = SND_PCM_FORMAT_FLOAT;                break;
        case 48: *fmt = SND_PCM_FORMAT_FLOAT64;              break;
        case 49: *fmt = SND_PCM_FORMAT_IEC958_SUBFRAME;      break;
        default:
            return -1;
    }
    return 0;
}
#endif

