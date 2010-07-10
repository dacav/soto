#ifndef __defined_headers_options_h
#define __defined_headers_options_h
#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>
#include <dacav/dacav.h>
#include <stdint.h>

#include "headers/thrd.h"

typedef struct {
    uint64_t period;        /**< Length of the period TODO decide measure
                             *   unit */
} opts_thrd_t;

/** Option keeping structure.
 *
 * @note The field opts_t::minpio will be added to the value returned by
 *       the sched_get_priority_min() system call.
 */
typedef struct {
    const char *device;         /**< PCM device */
    enum {
        MONO = 1, STEREO = 2
    } mode;                     /**< Number of input channels */
    unsigned rate;              /**< Sample rate; */
    snd_pcm_format_t format;    /**< Sampling input format; */
    int minprio;                /**< Priority for the thread having the
                                     longest sampling period; */

    dlist_t * threads;          /**< Sampling threads info. */
    size_t nthreads;            /**< Number of sampling threads */
} opts_t;

/** Parse the command line options
 *
 * @param opts The structure holding the options;
 * @param argc Main's argument counter;
 * @param argv Main's argument vector;
 * @return 0 on success, non-zero on fail.
 */
int opts_parse (opts_t *opts, int argc, char * const argv[]);

void opts_destroy (opts_t *opts);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_options_h

