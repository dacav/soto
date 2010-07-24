#ifndef __defined_headers_plotthread_h
#define __defined_headers_plotthread_h
#ifdef __cplusplus
extern "C" {
#endif

#include <thdacav/thdacav.h>

#include "headers/thrd.h"
#include "alsagw.h"

int plotth_subscribe (thrd_pool_t *pool, thdqueue_t *input,
                      const samp_info_t *samp);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_plotthread_h

