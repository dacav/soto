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

/** @file logging.h
 *
 * This file contains defines used for debugging and logging purposes.
 *
 * In order to enable debugging verbosity please run the configure script
 * with the '--enable-debug' flag.
 *
 */

#ifndef __defined_headers_logging_h
#define __defined_headers_logging_h
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdio.h>

#include "headers/config.h"

#ifdef DEBUG_ENABLED

/** Debugging macro for a simple string. */
#define DEBUG_MSG(str) \
        fprintf(stderr, "[%08X] " str "\n", (unsigned) pthread_self())

/** Debugging macro for a printf-like formt. */
#define DEBUG_FMT(fmt, ...) \
        fprintf(stderr, "[%08X] " fmt "\n", (unsigned) pthread_self(), __VA_ARGS__)

/** Debugging macro for struct timespec. */
#define DEBUG_TIMESPEC(str, remain)                                 \
    do if ((remain).tv_sec != 0 || (remain).tv_nsec != 0) {         \
        fprintf(stderr, "[%08X] " str " [sec=%u, nsec=%lu]\n",      \
                (unsigned) pthread_self(),                          \
                (unsigned) ((remain).tv_sec),                       \
                (remain).tv_nsec);                                  \
    } while (0)

#else

/** Debugging macro for a simple string. */
#define DEBUG_MSG(str)

/** Debugging macro for a printf-like formt. */
#define DEBUG_FMT(fmt, ...)

/** Debugging macro for struct timespec. */
#define DEBUG_TIMESPEC(str, remain)

#endif

/** Error logging macro for a simple string. */
#define ERR_MSG(str) \
        fprintf(stderr, "[%08X] " str "\n", (unsigned) pthread_self())

/** Error logging macro for a printf-like formt. */
#define ERR_FMT(fmt, ...) \
        fprintf(stderr, "[%08X] " fmt "\n", (unsigned) pthread_self(), __VA_ARGS__)

/** Logging macro for a simple string. */
#define LOG_MSG(str) \
        fprintf(stdout, "[%08X] " str "\n", (unsigned) pthread_self())

/** Logging macro for a printf-like formt. */
#define LOG_FMT(fmt, ...) \
        fprintf(stdout, "[%08X] " fmt "\n", (unsigned) pthread_self(), __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_logging_h

