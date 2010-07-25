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

#include <pthread.h>
#include <signal.h>

#include "headers/rtutils.h"
#include "headers/logging.h"

void rtutils_time_increment (struct timespec *target,
                             const struct timespec *val)
{
    long nano;

    target->tv_sec += val->tv_sec;
    nano = target->tv_nsec + val->tv_nsec;
    if (nano > SECOND_NS) {
        target->tv_sec ++;
        nano -= SECOND_NS;
    }
    target->tv_nsec = nano;
}

int rtutils_time_cmp (const struct timespec *s0,
                      const struct timespec *s1)
{
    if (s0->tv_sec == s1->tv_sec)
        return s0->tv_nsec > s1->tv_nsec ? -1 : 1;
    return s0->tv_sec > s1->tv_sec ? -1 : 1;
}

struct timespec rtutils_ns2time (uint64_t ns)
{
    struct timespec ret;
    ret.tv_sec = ns / SECOND_NS;
    ret.tv_nsec = ns % SECOND_NS;
    return ret;
}

