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
 *	Supported chart types: line, bar (grouped), and pie.
 *
 ****************************************************************************/

#ifndef _ichart_h
#define _ichart_h

#include "Ilib.h"

/**
 * @file Ichart.h
 * @brief Charting API for Ilib (line, bar and pie charts).
 */

/** Opaque chart handle. Create with ICreateChart(), free with IFreeChart(). */
typedef void *IChart;

/** Chart types. */
typedef enum {
  ICHART_LINE = 0, /* connected line chart (one line per series) */
  ICHART_BAR,      /* grouped vertical bar chart */
  ICHART_PIE       /* pie chart (uses the first series' values as slices) */
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
 * Add a data series. values[] holds count values; label and color are used in
 * the legend (and as the line/bar color). The data is copied. For a pie chart
 * the first series' values are the slice sizes.
 */
IError IChartAddSeries ( IChart chart, const char *label, const double *values,
  int count, IColor color );

/**
 * Render the chart to a newly created image (free it with IFreeImage()).
 * Returns NULL on failure.
 */
IImage IChartRender ( IChart chart );

#endif /* _ichart_h */
