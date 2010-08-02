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

#include "headers/logging.h"
#include "headers/constants.h"
#include "headers/rtutils.h"
#include "headers/plotting.h"
#include "headers/plotthread.h"
#include "headers/thrd.h"

static
int thread_cb (void *arg)
{
    plot_redraw((plot_t *)arg);
    return 0;
}

int plotth_subscribe (genth_t **handle, thrd_pool_t *pool,
                      plot_t *plot)
{
    thrd_info_t thi;

    thi.init = NULL;
    thi.callback = thread_cb;
    thi.destroy = NULL;
    thi.context = (void *) plot;

    /* Plotting thread starts immediatly */
    thi.delay.tv_sec = 0;
    thi.delay.tv_nsec = 0;

    thi.period.tv_sec = PLOT_PERIOD_SEC;
    thi.period.tv_nsec = PLOT_PERIOD_nSEC;

    return genth_subscribe(handle, pool, &thi);
}

