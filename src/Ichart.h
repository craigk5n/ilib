/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Ichart.h
 *
 * Charting on top of Ilib.
 *
 * Description:
 *	A small charting layer built entirely on the public Ilib drawing API.
 *	Create a chart, set its title/labels/font, add one or more data series,
 *	then render it to an ::IImage you can save or draw onto another image.
 *
 *	Supported chart types: line, bar (grouped or stacked, vertical or
 *	horizontal), pie, donut, area and scatter.
 *
 ****************************************************************************/

#ifndef _ichart_h
#define _ichart_h

#include "Ilib.h"

/**
 * @file Ichart.h
 * @brief Charting API for Ilib (line, bar, hbar, pie, donut, area, scatter).
 */

/** Opaque chart handle. Create with ICreateChart(), free with IFreeChart(). */
typedef void *IChart;

/** Chart types. */
typedef enum {
  ICHART_LINE = 0, /* connected line chart (one line per series) */
  ICHART_BAR,      /* grouped vertical bar chart */
  ICHART_PIE,      /* pie chart (uses the first series' values as slices) */
  ICHART_SCATTER,  /* scatter plot of (x, y) points (see IChartAddXYSeries) */
  ICHART_AREA,     /* line chart with the area under each line filled */
  ICHART_HBAR,     /* grouped horizontal bar chart */
  ICHART_DONUT     /* pie chart with a hole in the centre */
} IChartType;

/**
 * Create a chart of the given type and pixel size. Returns a handle or NULL.
 */
IChart ICreateChart ( IChartType type, unsigned int width,
  unsigned int height );

#define IFreeChart( c ) \
  _IFreeChart ( c );    \
  ( c ) = NULL;

/** Free a chart (use the IFreeChart() macro in client code). */
IError _IFreeChart ( IChart chart );

/** Set the chart title (drawn centered at the top; needs a font). */
IError IChartSetTitle ( IChart chart, const char *title );

/** Set the x- and y-axis labels (either may be NULL). Ignored for pie. */
IError IChartSetAxisLabels ( IChart chart, const char *x_label,
  const char *y_label );

/**
 * Set the font used for the title, axis labels, tick labels and legend. The
 * font is borrowed (not freed by the chart). With no font, text is skipped.
 */
IError IChartSetFont ( IChart chart, IFont font );

/** Set the background fill color (default white). */
IError IChartSetBackground ( IChart chart, IColor color );

/**
 * Set the category (x-axis) labels for line/bar charts, or the slice labels
 * for a pie chart. The strings are copied.
 */
IError IChartSetCategories ( IChart chart, const char **labels, int count );

/**
 * Fix the value (y) axis range. By default the range is auto-computed from the
 * data (bar charts always include 0).
 */
IError IChartSetRange ( IChart chart, double ymin, double ymax );

/**
 * For a bar chart, stack the series on top of each other (per category)
 * instead of drawing them grouped side by side. Non-zero enables stacking.
 * Intended for non-negative data; not combined with a log scale.
 */
IError IChartSetStacked ( IChart chart, int stacked );

/**
 * Use a logarithmic (base-10) value axis. The data and range must be positive;
 * tick marks are placed at powers of ten. Applies to line, bar (non-stacked),
 * area and scatter charts.
 */
IError IChartSetLogScale ( IChart chart, int on );

/**
 * Draw each data value as a small text label (above points/bars, inside
 * stacked bar segments, and on pie slices). Requires a font; off by default.
 */
IError IChartSetValueLabels ( IChart chart, int on );

/**
 * Draw a marker (filled dot) at each point of line and area charts. On by
 * default; pass 0 for clean lines with no point markers.
 */
IError IChartSetMarkers ( IChart chart, int on );

/** Show or hide the value/category gridlines (on by default). */
IError IChartSetGrid ( IChart chart, int on );

/** Show or hide the series/category legend (on by default). */
IError IChartSetLegend ( IChart chart, int on );

/**
 * Add a data series. values[] holds count values; label and color are used in
 * the legend (and as the line/bar color). The data is copied. For a pie chart
 * the first series' values are the slice sizes.
 */
IError IChartAddSeries ( IChart chart, const char *label, const double *values,
  int count, IColor color );

/**
 * Add a series with explicit x and y values, for a scatter chart. xvalues[]
 * and yvalues[] each hold count values; both are copied. (Line/bar/area charts
 * ignore the x values and use the category index; scatter charts require
 * them.)
 */
IError IChartAddXYSeries ( IChart chart, const char *label,
  const double *xvalues, const double *yvalues, int count, IColor color );

/**
 * Render the chart to a newly created image (free it with IFreeImage()).
 * Returns NULL on failure.
 */
IImage IChartRender ( IChart chart );

#endif /* _ichart_h */
