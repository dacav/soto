#ifndef __defined_headers_rtutils_h
#define __defined_headers_rtutils_h
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <assert.h>

#define SECOND_NS 1000000000UL

static inline
void rtutils_get_now (struct timespec *n)
{
    clock_gettime(CLOCK_MONOTONIC, n);
}

static inline
void rtutils_wait (const struct timespec *delay)
{
    assert(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                           delay, NULL) == 0);
}

/* Make absolute a certain time wait by adding some 'current time' */
void rtutils_time_increment (struct timespec *target,
                             const struct timespec *val);

/* Comparsion between timespec structures, allows to determine which
 * period is greater
 */
int rtutils_time_cmp (const struct timespec *s0,
                      const struct timespec *s1);

static inline
int rtutils_time_iszero (const struct timespec *s)
{
    return s->tv_sec == 0 && s->tv_nsec == 0;
}


#ifdef __cplusplus
}
#endif
#endif // __defined_headers_rtutils_h

