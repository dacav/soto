#include "headers/rtutils.h"

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

