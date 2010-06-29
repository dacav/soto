#ifndef __defined_headers_options_h
#define __defined_headers_options_h
#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>

/** Option keeping structure */
typedef struct {
    const char *device;         /**< PCM device */
    enum {
        MONO = 1, STEREO = 2
    } mode;                     /**< Number of input channels */
    unsigned rate;              /**< Sample rate */
    snd_pcm_format_t format;
} soto_opts_t;

/** Parse the command line options
 *
 * @param opts The structure holding the options;
 * @param argc Main's argument counter;
 * @param argv Main's argument vector;
 * @return 0 on success, non-zero on fail.
 */
int soto_opts_parse (soto_opts_t *opts, int argc,
                     char * const argv[]);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_options_h

