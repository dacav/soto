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

/*

   This module provieds some handy shortcuts for real-time related data
   structures (like 'struct timespec').

 */

#ifndef __defined_headers_rtutils_h
#define __defined_headers_rtutils_h
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <assert.h>

#define SECOND_NS 1000000000UL

/** Current time on the monotonic clock
 *
 * @param n The address of the 'struct timespec' where the data will be
 *          stored.
 */
static inline
void rtutils_get_now (struct timespec *n)
{
    clock_gettime(CLOCK_MONOTONIC, n);
}

/** Sleep until a certain absolute delay expires
 *
 * @param delay The execution delay.
 */
static inline
void rtutils_wait (const struct timespec *delay)
{
    assert(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                           delay, NULL) == 0);
}

/** Sum a time specification to another.
 *
 * This is an increment operator for 'struct timespec'
 *
 * @param target The time which will be incremented;
 * @param val The value to add as increment.
 */
void rtutils_time_increment (struct timespec *target,
                             const struct timespec *val);

/** Comparsion between 'struct timespec'.
 *
 * Allows to determine which period is greater (strcmp-like return
 * value).
 *
 * @param s0 First time specification;
 * @param s1 Second time specification;
 * @return An integer less than, equal to, or greater than zero if s0 is
 *         found, respectively, to be less than, to match, or be greater
 *         than s1.
 */
int rtutils_time_cmp (const struct timespec *s0,
                      const struct timespec *s1);

/** Null time predicate.
 *
 * @param s The 'struct timespec' to test;
 * @return 1 if s corresponds to time zero, 0 otherwise.
 */
static inline
int rtutils_time_iszero (const struct timespec *s)
{
    return s->tv_sec == 0 && s->tv_nsec == 0;
}

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_rtutils_h
