/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IchartP.h
 *
 * Private chart structures. Included only by the charting sources.
 *
 ****************************************************************************/

#ifndef _ichartp_h
#define _ichartp_h

#include "Ichart.h"
#include "Ilib.h"

#define IMAGIC_CHART 729 /* magic value for chart-handle validation */

/** One data series. */
typedef struct {
  char *label;     /* owned copy, or NULL */
  double *values;  /* owned array (y values) */
  double *xvalues; /* owned array (x values for scatter), or NULL */
  int count;
  IColor color;
} IChartSeries;

/** Concrete chart object behind the opaque ::IChart handle. */
typedef struct {
  unsigned int magic;
  IChartType type;
  int width, height;
  char *title;             /* owned, or NULL */
  char *x_label, *y_label; /* owned, or NULL */
  IFont font;              /* borrowed (not owned); may be NULL */
  IColor background;
  char **categories; /* owned array of owned strings */
  int ncategories;
  IChartSeries *series; /* owned */
  int nseries;
  int auto_range; /* 1 = derive y range from the data */
  double ymin, ymax;
  int stacked;      /* bar charts: stack series instead of grouping */
  int log_scale;    /* logarithmic value (y) axis */
  int value_labels; /* draw each data value as a text label */
  int markers;      /* line/area: draw a marker at each point (default on) */
  int show_grid;    /* draw gridlines (default on) */
  int show_legend;  /* draw the legend (default on) */
  int bg_gradient;  /* fill background with a top->bottom gradient */
  IColor bg_color2; /* bottom color of the background gradient */
  int bar_radius;   /* corner radius for bar/hbar bars (0 = square) */
} IChartP;

#endif /* _ichartp_h */
