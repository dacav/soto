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

#include "headers/alsagw.h"
#include "headers/rtutils.h"
#include "headers/logging.h"

/* All error OR-ed, for cleanup, used by strerr */
#define SAMP_ERR_ALL \
    ( SAMP_ERR_LIBRARY | SAMP_ERR_RATE | SAMP_ERR_PERIOD )

/* Information about this structure can be retrieved by looking at getters
 * doxygen */
struct samp {
    snd_pcm_t *pcm;
    unsigned rate;
    snd_pcm_uframes_t nframes;
    struct timespec period;
};

static
int init_soundcard (snd_pcm_t *handle, unsigned *rate,
                    snd_pcm_uframes_t *nframes, unsigned *period)
{
    snd_pcm_hw_params_t *hwparams;
    int err;

    snd_pcm_hw_params_alloca(&hwparams);

    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) return err;

    err = snd_pcm_hw_params_set_rate_near(handle, hwparams, rate, NULL);
    if (err < 0) return err;

    err = snd_pcm_hw_params_set_access(handle, hwparams,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) return err;

    err = snd_pcm_hw_params_set_format(handle, hwparams,
                                       SND_PCM_FORMAT_S16_LE);
    if (err < 0) return err;

    /*
    err = snd_pcm_hw_params_set_channels(handle, hwparams, channels);
    if (err < 0) return err;
    */

    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) return err;

    err = snd_pcm_hw_params_get_period_size_min(hwparams, nframes, NULL);
    if (err < 0) return err;

    err = snd_pcm_hw_params_get_period_time(hwparams, period, NULL);
    if (err < 0) return err;

    return 0;
}

const struct timespec * samp_get_period (const samp_t *samp)
{
    return &samp->period;
}

snd_pcm_uframes_t samp_get_nframes (const samp_t *samp)
{
    return samp->nframes;
}

unsigned samp_get_rate (const samp_t *samp)
{
    return samp->rate;
}

samp_t * samp_new (const char *device, unsigned rate, int *err)
{
    samp_t *s;
    snd_pcm_uframes_t nframes;
    unsigned period;
    snd_pcm_t *pcm;
    int e;
    
    if ((e = snd_pcm_open(&pcm, device, SND_PCM_STREAM_CAPTURE,
                          SND_PCM_NONBLOCK) != 0)) {
        *err = e;
        return NULL;
    }

    if ((e = init_soundcard(pcm, &rate, &nframes, &period)) != 0) {
        *err = e;
        return NULL;
    }

    s = (samp_t *) calloc(1, sizeof(samp_t));
    assert(s);

    s->pcm = pcm;
    s->nframes = nframes;
    s->period = rtutils_ns2time(1000 * period);
    s->rate = rate;

    return s;
}

void samp_destroy (samp_t *s)
{
    if (s != NULL) {
        snd_pcm_close(s->pcm);
        free(s);
    }
}

int samp_read (samp_t *samp, samp_frame_t *buffer,
               snd_pcm_uframes_t bufsize, int maxwait)
{
    int nread;
    snd_pcm_t *pcm = samp->pcm;

    nread = (int) snd_pcm_readi(pcm, buffer, bufsize);
    if (nread > 0) {
        /* Everything worked correctly. */
        return nread;
    }

    /* Note: alsa errors are unix ones, but negative */
    switch (nread) {
        case -EPIPE:
            LOG_MSG("Got overrun");
            if (snd_pcm_recover(pcm, nread, 0)) {
                LOG_MSG("Overrun handling failure");
            }
            nread = 1;
        case -EAGAIN:
            /* Smelly undocumented failure, which turns out to be
             * recoverable with a little waiting before trying again. This
             * can be achieved by snd_pcm_wait, which btw requires the
             * value to be expressed in microseconds (hence maxwait/10).
             */
            LOG_MSG("Waiting resource");
            nread = snd_pcm_wait(pcm, maxwait / 10);
    }

    /* After the recover we retry to read data once, if it fails again we
     * just skip to next activation and abort this job */
    switch (nread) {
        case 1:
            return (int) snd_pcm_readi(pcm, buffer, bufsize);
        case 0:
            return -EAGAIN;
        case -EPIPE:
            return snd_pcm_recover(pcm, nread, 0);
        default:
            /* Everything is badly documented here. Let the snd_strerr
             * decide what is this, I did the best effort to recover. */
            return nread;
    }
}

