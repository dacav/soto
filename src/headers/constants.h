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
#define PLOT_MIN_Y (2 * INT16_MAX)
#define PLOT_MAX_Y (2 * INT16_MIN)
#define PLOT_OFFSET_UP INT16_MAX
#define PLOT_OFFSET_DOWN INT16_MIN


/* Multiplication factor (plotthread.c): the period of the plotter will be
 * a multiple of the sampling period w.r.t. this constant */
#define PLOT_PERIOD_TIMES 10

/* Number of sampling used for average */
#define PLOT_AVERAGE_LEN  10

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_constants_h

