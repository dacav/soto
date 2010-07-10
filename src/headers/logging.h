#ifndef __defined_headers_logging_h
#define __defined_headers_logging_h
#ifdef __cplusplus
extern "C" {
#endif

#include "headers/config.h"

#ifdef DEBUG_ENABLED
#define DEBUG_MSG(str) \
        fprintf(stderr, "[%08X] " str "\n", (unsigned) pthread_self())
#define DEBUG_FMT(fmt, ...) \
        fprintf(stderr, "[%08X] " fmt "\n", (unsigned) pthread_self(), __VA_ARGS__)
#else
#define DEBUG_MSG
#define DEBUG_FMT
#endif

#define LOG_MSG(str) \
        fprintf(stderr, "[%08X] " str "\n", (unsigned) pthread_self())
#define LOG_FMT(fmt, ...) \
        fprintf(stderr, "[%08X] " fmt "\n", (unsigned) pthread_self(), __VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_logging_h

