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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sched.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <dacav/dacav.h>

#include "headers/config.h"
#include "headers/alsagw.h"
#include "headers/logging.h"
#include "headers/sampthread.h"
#include "headers/thrd.h"
#include "headers/plotting.h"
#include "headers/plotthread.h"
#include "headers/signal_show.h"
#include "headers/spectrum_show.h"
#include "headers/options.h"

/* Information allocated for each thread, contains statistical information
 * about the real-time thread. */
struct rtstat_show {
    const thrd_rtstats_t *stats;
    const char * name;
};

struct main_data {
    opts_t *opts;
    thrd_pool_t *pool;
    alsagw_t *sampler;

    plot_t *spectrum;
    plot_t *signal;

    /* This list is used as stack: it will contain all threads handlers in
     * inverse-order of deallocation, thus by pop-ing elements I obtain
     * the correct deallocation order. */
    dlist_t *threads;

    /* This list contains instances of the rtstat_show structure. */
    dlist_t *stats;

    #ifndef RT_DISABLE
    bool memlock;
    #endif
};

static inline
void drop_statistics (struct main_data *data)
{
    dlist_free(data->stats, free);
}

static
void show_statistics (struct main_data *data)
{
    diter_t *iter;

    iter = dlist_iter_new(&data->stats);
    while (diter_hasnext(iter)) {
        struct rtstat_show *s = diter_next(iter);
        const thrd_rtstats_t * rts = s->stats;

        LOG_FMT("\tStatistics for thread '%s':", s->name);
        LOG_FMT("\t\tAvg response time (ns):         %10llu",
                (unsigned long long) (rts->response_times /
                                     rts->n_executions));
        LOG_FMT("\t\tWorst case response time (ns):  %10llu",
                (unsigned long long) rts->wcrt);
        LOG_FMT("\t\tNumber of executions:           %10llu",
                (unsigned long long) rts->n_executions);
        LOG_FMT("\t\tNumber of deadline misses:      %10llu", 
                (unsigned long long) rts->dmiss_count);
        LOG_FMT("\t\tDeadline miss ratio (%%)         %10G\n",
                100 * (double)((double)(rts->dmiss_count) /
                                        rts->n_executions));
        diter_remove(iter, free);
    }
    dlist_iter_free(iter);
}

static
void exit_handler (int xval, void *context)
{
    struct main_data *data = (struct main_data *)context;

    DEBUG_FMT("Exiting on %s", xval == EXIT_SUCCESS ?
                               "success" : "failure");

    if (data->opts) opts_destroy(data->opts);

    LOG_MSG("Sending kill to all threads...");
    while (!dlist_empty(data->threads)) {
        void *handle;

        data->threads = dlist_pop(data->threads, (void **)&handle);
        genth_sendkill((genth_t *)handle);
    }
    dlist_free(data->threads, NULL);

    if (xval == EXIT_SUCCESS) {
        LOG_MSG("EXECUTION STATISTICS");
        show_statistics(data);
    } else {
        drop_statistics(data);
    }

    LOG_MSG("Waiting until they're dead (WARNING: if you just closed");
    LOG_MSG("the window, you've better to kill the program explicitly).");
    if (data->pool) thrd_destroy(data->pool);
    if (data->sampler) alsagw_destroy(data->sampler);
    if (data->spectrum) plot_destroy(data->spectrum);
    if (data->signal) plot_destroy(data->signal);

    #ifndef RT_DISABLE
        if (data->memlock) munlockall();
    #endif

    LOG_MSG("Goodbye");
}

static
void sigterm_handler (int sig)
{
    exit(EXIT_FAILURE);
}

static
struct rtstat_show * rtstat_show_new (const thrd_rtstats_t *stats,
                                      const char *name)
{   
    struct rtstat_show *ret;
    ret = (struct rtstat_show *) malloc(sizeof(struct rtstat_show));
    assert(ret);
    ret->stats = stats;
    ret->name = name;

    return ret;
}

int main (int argc, char **argv)
{
    struct main_data data;
    genth_t *sampth;
    const thrd_rtstats_t * rtstats;
    int err;
    unsigned run_for;

    signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    memset(&data, 0, sizeof(struct main_data));
    data.threads = dlist_new();
    data.stats = dlist_new();

    if ((data.opts = opts_parse(argc, argv)) == NULL) {
        exit(EXIT_FAILURE);
    }

    on_exit(exit_handler, (void *) &data);

    data.pool = thrd_new(opts_get_minprio(data.opts));
    data.sampler = alsagw_new(opts_get_device(data.opts),
                              opts_get_rate(data.opts),
                              &err);
    if (data.sampler == NULL) {
        ERR_FMT("Unable to start Alsa: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    rtstats = sampth_subscribe(&sampth, data.pool, data.sampler,
                               opts_get_buffer_scale(data.opts));
    if (rtstats == NULL) {
        ERR_FMT("Unable to start Sampler: %s",
                thrd_strerr(data.pool, thrd_interr(data.pool)));
        exit(EXIT_FAILURE);
    }
    data.threads = dlist_push(data.threads, sampth);
    data.stats = dlist_push(data.stats, rtstat_show_new(rtstats,
                            "Sampling"));

    if (opts_spectrum_shown(data.opts)) {
        genth_t *handle;
        specth_graphics_t spec_graphs;

        data.spectrum = plot_new(4, sampth_get_size(sampth));
        rtstats = plotth_subscribe(&handle, data.pool, data.spectrum);
        if (rtstats == NULL) {
            ERR_FMT("Unable to start Spectrum Plotter: %s",
                    thrd_strerr(data.pool, thrd_interr(data.pool)));
            exit(EXIT_FAILURE);
        }
        data.threads = dlist_push(data.threads, handle);
        data.stats = dlist_push(data.stats, rtstat_show_new(rtstats,
                                "Spectrum's plot updater"));

        spec_graphs.r0 = plot_new_graphic(data.spectrum);
        spec_graphs.i0 = plot_new_graphic(data.spectrum);
        spec_graphs.r1 = plot_new_graphic(data.spectrum);
        spec_graphs.i1 = plot_new_graphic(data.spectrum);

        rtstats = specth_subscribe(&handle, data.pool, sampth,
                                   &spec_graphs);
        if (rtstats == NULL) {
            ERR_FMT("Unable to start Spectrum Analizer: %s",
                    thrd_strerr(data.pool, thrd_interr(data.pool)));
            exit(EXIT_FAILURE);
        }
        data.threads = dlist_push(data.threads, handle);
        data.stats = dlist_push(data.stats, rtstat_show_new(rtstats,
                                "Spectrum show"));
    }

    if (opts_signal_shown(data.opts)) {
        genth_t *handle;

        data.signal = plot_new(2, sampth_get_size(sampth));
        rtstats = plotth_subscribe(&handle, data.pool, data.signal);
        if (rtstats == NULL) {
            ERR_FMT("Unable to start Signal Plotter: %s",
                    thrd_strerr(data.pool, thrd_interr(data.pool)));
            exit(EXIT_FAILURE);
        }
        data.threads = dlist_push(data.threads, handle);
        data.stats = dlist_push(data.stats, rtstat_show_new(rtstats,
                                "Signal's plot updater"));

        rtstats = signth_subscribe(&handle, data.pool, sampth,
                                   plot_new_graphic(data.signal),
                                   plot_new_graphic(data.signal));
        if (rtstats == NULL) {
            ERR_FMT("Unable to start Signal Analyzer: %s",
                    thrd_strerr(data.pool, thrd_interr(data.pool)));
            exit(EXIT_FAILURE);
        }
        data.threads = dlist_push(data.threads, handle);
        data.stats = dlist_push(data.stats, rtstat_show_new(rtstats,
                                "Signal show"));
    }

    /* Locking memory */
    #ifndef RT_DISABLE
        data.memlock = false;
        if (mlockall( MCL_CURRENT | MCL_FUTURE )) {
            ERR_FMT("Unable to lock memory: %s", strerror(errno));
            ERR_MSG("You need more privileges to do this!");
            exit(EXIT_FAILURE);
        }
        data.memlock = true;
    #endif

    if (thrd_start(data.pool)) {
        ERR_FMT("Unable to start Signal Analyzer: %s",
                thrd_strerr(data.pool, thrd_interr(data.pool)));
        exit(EXIT_FAILURE);
    }

    run_for = opts_get_run_for(data.opts);
    if (run_for) sleep(opts_get_run_for(data.opts));
    else pause();

    exit(EXIT_SUCCESS);
}
