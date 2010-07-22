#ifndef __defined_headers_constants_h
#define __defined_headers_constants_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SECOND_NS 1000000000UL

/* Startup delay for all threads. This should be enough for completing all
 * the operations, I guess.
 *
 * TODO: make it configurable via auto-tools.
 */
#define STARTUP_DELAY_SEC  0
#define STARTUP_DELAY_nSEC 500000

/* Size of the graphical window used for plotting */
#define PLOT_BITMAPSIZE "300x150"
#define PLOT_MIN_Y INT16_MIN
#define PLOT_MAX_Y INT16_MAX

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_constants_h

