#ifndef __defined_headers_audio_h
#define __defined_headers_audio_h
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SUCCESS = 0,
    NO_RTSCHED = 1
} audio_err_t; 

typedef 

audio_err_t audio_init (opts_t *opts);

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_audio_h

