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

#include <stdint.h>
#include <fftw3.h>

#include "headers/spectrum_show.h"
#include "headers/logging.h"
#include "headers/alsagw.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/sampthread.h"

struct fourier {
    /* Input vector, sized buflen * sizeof(doubles). */
    double *in;

    /* Complex output vector,
     * sized (buflen/2 + 1) * sizeof(fftw_complex). */
    fftw_complex *out;

    /* Computational plan. */
    fftw_plan plan;
};

struct specth_data {
    alsagw_frame_t *buffer;
    snd_pcm_uframes_t buflen;
    genth_t *sampth;

    specth_graphics_t graphs;

    struct fourier ft;      /* Fourier transform data */
};

static
int destroy_cb (void *arg)
{
    struct specth_data *ctx = (struct specth_data *)arg;

    free(ctx->buffer);
    fftw_destroy_plan(ctx->ft.plan);
    fftw_free(ctx->ft.in);
    fftw_free(ctx->ft.out);
    free(arg);

    return 0;
}

static inline
int16_t denormalize (double val)
{
    return (int16_t)((double)INT16_MAX * val);
}

static
void build_spectrum (struct fourier *ft, alsagw_frame_t *buffer,
                     size_t buflen, plotgr_t *imag, plotgr_t *real,
                     double (*normalize) (alsagw_frame_t *))
{
    size_t nfreqs = buflen >> 1 ;
    int i, j;

    for (i = 0; i < buflen; i ++) {
        ft->in[i] = normalize(&buffer[i]);
    }
    fftw_execute(ft->plan);
    j = nfreqs; i = 0;
    /* Negative part of the spectrum (j down to 0) */
    while (j >= 0) {
        plot_graphic_set(real, i, denormalize(ft->out[j][0]));
        plot_graphic_set(imag, i, denormalize(ft->out[j][1]));
        j --; i ++;
    }
    j = 1;
    /* Positive part of the spectrum (j up to nfreqs) */
    while (j < nfreqs) {
        plot_graphic_set(real, i, denormalize(ft->out[j][0]));
        plot_graphic_set(imag, i, denormalize(ft->out[j][1]));
        j ++; i ++;
    }
}

static
double normalize_ch0 (alsagw_frame_t *f)
{
    return (double)(f->ch0) / INT16_MAX;
}

static
double normalize_ch1 (alsagw_frame_t *f)
{
    return (double)(f->ch1) / INT16_MAX;
}

static
int thread_cb (void *arg)
{
    struct specth_data *ctx = (struct specth_data *)arg;

    sampth_get_samples(ctx->sampth, ctx->buffer);
    build_spectrum(&ctx->ft, ctx->buffer, ctx->buflen,
                   ctx->graphs.i0, ctx->graphs.r0,
                   normalize_ch0);
    build_spectrum(&ctx->ft, ctx->buffer, ctx->buflen,
                   ctx->graphs.i1, ctx->graphs.r1,
                   normalize_ch1);
    return 0;
}

int specth_subscribe (genth_t **handle, thrd_pool_t *pool,
                      genth_t *sampth, const specth_graphics_t *graphs)
{
    struct specth_data *ctx;
    thrd_info_t thi;
    int err;
    size_t buflen;

    thi.init = NULL;
    thi.callback = thread_cb;
    thi.destroy = destroy_cb;

    ctx = (struct specth_data *) calloc(1, sizeof(struct specth_data));
    assert(ctx);
    thi.context = (void *) ctx;

    /* Common startup delay. */
    thi.delay.tv_sec = SAMP_STARTUP_DELAY_SEC;
    thi.delay.tv_nsec = SAMP_STARTUP_DELAY_nSEC;

    /* The startup delay must be incremented in order to allow the
     * sampling thread to fill at least one buffer. The same value is used
     * to set the period. */
    rtutils_time_increment(&thi.delay, sampth_get_period(sampth));
    rtutils_time_copy(&thi.period, &thi.delay);

    memcpy(&ctx->graphs, graphs, sizeof(specth_graphics_t));
    ctx->buflen = buflen = sampth_get_size(sampth);
    ctx->buffer = calloc(buflen, sizeof(alsagw_frame_t));
    ctx->sampth = sampth;
    ctx->ft.in = (double *) fftw_malloc(sizeof(double) * buflen);

    /* As the fftw documentation says, since this is a real->complex
     * transformation, we need only N/2 slots for the output vector plus
     * the DC component. */
    ctx->ft.out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) *
                                               ((buflen >> 1) + 1));
    ctx->ft.plan = fftw_plan_dft_r2c_1d(buflen, ctx->ft.in, ctx->ft.out,
                                        FFTW_MEASURE);

    if ((err = genth_subscribe(handle, pool, &thi)) != 0) {
        free(ctx->buffer);
        free(ctx);
    }
    return err;
}

