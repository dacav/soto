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

/** @file plotting.h */
/** @addtogroup BizPlotting */
/*@{*/

#ifndef __defined_headers_plotting_h
#define __defined_headers_plotting_h
#ifdef __cplusplus
extern "C" {
#endif

#include <dacav/dacav.h>
#include <plot.h>
#include <stdint.h>

/** @brief Plotter opaque type. */
typedef struct plot plot_t;

/** @brief Plotter graphic opaque type */
typedef struct graphic plotgr_t;

/** @brief Plotter constructor.
 *
 * @note This function spawns a X11 window on which the plot will be
 *       displayed.
 *
 * @param n The number of graphics that shall be drawn on the canvas;
 * @param max_x The maximum accepted value for the x axis.
 *
 * @return The newly allocated plot instance.
 */
plot_t * plot_new (size_t n, unsigned max_x);

/** @brief Add a new graphic.
 *
 * @param p The plotter.
 *
 * @return The new graphic or NULL if the maximum number of plots has been
 *         reached.
 */
plotgr_t * plot_new_graphic (plot_t *p);

/** @brief Write a value on a graphic.
 *
 * In order to visualize modification plot_redraw() must be called.
 *
 * @param g The graphic to write on;
 * @param pos The position to modify;
 * @param val The new value to write.
 *
 * @note The array position goes from 0 to the value provided as max_t
 *       parameter for plot_new().
 */
void plot_graphic_set (plotgr_t *g, unsigned pos, int16_t val);

/** @brief Update the window by redrawing.
 *
 * @param p The plot to be redrawn.
 */
void plot_redraw(plot_t *p);

/** @brief Plotter Destructor
 *
 * @param p The plotter to be destroyed.
 */
void plot_destroy (plot_t *p);

/*@}*/

#ifdef __cplusplus
}
#endif
#endif // __defined_headers_plotting_h

