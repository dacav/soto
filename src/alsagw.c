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

static
int init_soundcard (snd_pcm_t *handle, const samp_info_t *spec,
                    unsigned *rate, snd_pcm_uframes_t *nframes)
{
    snd_pcm_hw_params_t *hwparams;
    int dir = 0, err;

    snd_pcm_hw_params_alloca(&hwparams);

    *rate = spec->rate;
    err = snd_pcm_hw_params_set_rate_near(handle, hwparams, rate, &dir);
    if (err < 0) return err;

    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) return err;

    err = snd_pcm_hw_params_set_access(handle, hwparams,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) return err;

    /* Only SND_PCM_FORMAT_S16_LE supported, for the moment */
    err = snd_pcm_hw_params_set_format(handle, hwparams,
                                       SND_PCM_FORMAT_S16_LE);
    if (err < 0) return err;

    err = snd_pcm_hw_params_set_channels(handle, hwparams, spec->channels);
    if (err < 0) return err;

    err = snd_pcm_hw_params_get_buffer_size_min(hwparams, nframes);
    if (err < 0) return err;

    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) return err;

    return 0;
}

/* Returns the period for the sampling thread based on the given sampling
 * specification. */
static
struct timespec build_period (const samp_info_t *spec)
{
    uint64_t sample_time = SECOND_NS / spec->rate;
    return rtutils_ns2time(sample_time * spec->nsamp);
}

const struct timespec * samp_get_period (const samp_t *samp)
{
    return &samp->period;
}

snd_pcm_uframes_t samp_get_nframes (const samp_t *samp)
{
    return samp->nframes;
}

snd_pcm_t * samp_get_pcm (const samp_t *samp)
{
    return samp->pcm;
}

int samp_init (samp_t *s, const samp_info_t *spec, samp_policy_t accept)
{
    unsigned rate;
    snd_pcm_uframes_t nframes;
    
    memset((void *)s, 0, sizeof(samp_t));
    s->period = build_period(spec);

    if ((s->err = snd_pcm_open(&s->pcm, spec->device,
                               SND_PCM_STREAM_CAPTURE,
                               SND_PCM_NONBLOCK) != 0)) {
        s->status |= SAMP_ERR_LIBRARY;
        return -1;
    }
    if ((s->err = init_soundcard(s->pcm, spec, &rate, &nframes)) != 0) {
        s->status |= SAMP_ERR_LIBRARY;
        return -1;
    }
    if ((rate != spec->rate) && (accept & SAMP_ACCEPT_RATE) == 0) {
        s->status |= SAMP_ERR_RATE;
        return -1;
    }
    s->nframes = nframes;

    return 0;
}

samp_err_t samp_interr (samp_t *s)
{
    uint8_t status = s->status;
    s->status &= ~SAMP_ERR_ALL;
    return status & SAMP_ERR_ALL;
}

const char * samp_strerr (samp_t *s, samp_err_t err)
{
    switch (err) {
        case SAMP_ERR_LIBRARY:
            return strerror(s->err);
        case SAMP_ERR_RATE:
            return "Rate has been modified";
        case SAMP_ERR_PERIOD:
            return "Period has been modified";
    }
    return "Unknown";
}

void samp_destroy (samp_t *s)
{
    snd_pcm_close(s->pcm);
}

