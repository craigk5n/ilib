/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IChart.c
 *
 * Charting on top of Ilib (line, bar and pie charts).
 *
 * Description:
 *	Builds a chart object from a title, axis labels, categories and one or
 *	more data series, then renders it to an image using only the public Ilib
 *	drawing API (axes, gridlines, anti-aliased lines, filled rectangles and
 *	pie wedges, and text via an optional font).
 *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Ichart.h"
#include "IchartP.h"
#include "Ilib.h"
#include "IlibP.h" /* _IGetColor, to derive translucent area-fill colors */

/* Plot-area rectangle (inclusive pixel bounds) and value/x ranges. */
typedef struct {
  int x0, y0, x1, y1;
  double ymin, ymax;
  double xmin, xmax; /* used by scatter charts */
  int log_scale;     /* logarithmic value (y) axis */
} IChartLayout;

/* ------------------------------------------------------------------ utils */

static char *_chart_strdup ( const char *s )
{
  char *d;
  size_t n;
  if ( !s )
    return ( NULL );
  n = strlen ( s ) + 1;
  d = (char *) malloc ( n );
  if ( d )
    memcpy ( d, s, n );
  return ( d );
}

static IChartP *_chart_valid ( IChart chart )
{
  IChartP *c = (IChartP *) chart;
  if ( !c || c->magic != IMAGIC_CHART )
    return ( NULL );
  return ( c );
}

/* A categorical colour palette (Tableau-10-ish), allocated once. */
static IColor _chart_palette ( int i )
{
  static const unsigned char rgb[10][3] = {
    { 0x4e, 0x79, 0xa7 }, { 0xf2, 0x8e, 0x2b }, { 0x59, 0xa1, 0x4f },
    { 0xe1, 0x57, 0x59 }, { 0x76, 0xb7, 0xb2 }, { 0xed, 0xc9, 0x48 },
    { 0xb0, 0x7a, 0xa1 }, { 0xff, 0x9d, 0xa7 }, { 0x9c, 0x75, 0x5f },
    { 0xba, 0xb0, 0xac } };
  static IColor cache[10];
  static int init = 0;
  if ( !init ) {
    int k;
    for ( k = 0; k < 10; k++ )
      cache[k] = IAllocColor ( rgb[k][0], rgb[k][1], rgb[k][2] );
    init = 1;
  }
  if ( i < 0 )
    i = 0;
  return ( cache[i % 10] );
}

static IColor _chart_grid_color ( void )
{
  static IColor grid;
  static int init = 0;
  if ( !init ) {
    grid = IAllocColor ( 220, 220, 220 );
    init = 1;
  }
  return ( grid );
}

/* ------------------------------------------------------------- lifecycle */

IChart ICreateChart ( IChartType type, unsigned int width,
  unsigned int height )
{
  IChartP *c;
  if ( width == 0 || height == 0 )
    return ( NULL );
  c = (IChartP *) calloc ( 1, sizeof ( IChartP ) );
  if ( !c )
    return ( NULL );
  c->magic = IMAGIC_CHART;
  c->type = type;
  c->width = (int) width;
  c->height = (int) height;
  c->background = IAllocColor ( 255, 255, 255 );
  c->auto_range = 1;
  return ( (IChart) c );
}

IError _IFreeChart ( IChart chart )
{
  IChartP *c = _chart_valid ( chart );
  int i;
  if ( !c )
    return ( IInvalidChart );

  free ( c->title );
  free ( c->x_label );
  free ( c->y_label );
  for ( i = 0; i < c->ncategories; i++ )
    free ( c->categories[i] );
  free ( c->categories );
  for ( i = 0; i < c->nseries; i++ ) {
    free ( c->series[i].label );
    free ( c->series[i].values );
    free ( c->series[i].xvalues );
  }
  free ( c->series );
  c->magic = 0;
  free ( c );
  return ( INoError );
}

/* --------------------------------------------------------------- setters */

IError IChartSetTitle ( IChart chart, const char *title )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  free ( c->title );
  c->title = _chart_strdup ( title );
  return ( INoError );
}

IError IChartSetAxisLabels ( IChart chart, const char *x_label,
  const char *y_label )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  free ( c->x_label );
  free ( c->y_label );
  c->x_label = _chart_strdup ( x_label );
  c->y_label = _chart_strdup ( y_label );
  return ( INoError );
}

IError IChartSetFont ( IChart chart, IFont font )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  c->font = font;
  return ( INoError );
}

IError IChartSetBackground ( IChart chart, IColor color )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  c->background = color;
  return ( INoError );
}

IError IChartSetCategories ( IChart chart, const char **labels, int count )
{
  IChartP *c = _chart_valid ( chart );
  int i;
  if ( !c )
    return ( IInvalidChart );
  if ( !labels || count < 0 )
    return ( IInvalidArgument );

  for ( i = 0; i < c->ncategories; i++ )
    free ( c->categories[i] );
  free ( c->categories );
  c->categories = NULL;
  c->ncategories = 0;
  if ( count == 0 )
    return ( INoError );

  c->categories = (char **) calloc ( (size_t) count, sizeof ( char * ) );
  if ( !c->categories )
    return ( IInvalidArgument );
  for ( i = 0; i < count; i++ )
    c->categories[i] = _chart_strdup ( labels[i] );
  c->ncategories = count;
  return ( INoError );
}

IError IChartSetRange ( IChart chart, double ymin, double ymax )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  if ( !( ymax > ymin ) )
    return ( IInvalidArgument );
  c->ymin = ymin;
  c->ymax = ymax;
  c->auto_range = 0;
  return ( INoError );
}

IError IChartSetStacked ( IChart chart, int stacked )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  c->stacked = stacked ? 1 : 0;
  return ( INoError );
}

IError IChartSetLogScale ( IChart chart, int on )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  c->log_scale = on ? 1 : 0;
  return ( INoError );
}

IError IChartSetValueLabels ( IChart chart, int on )
{
  IChartP *c = _chart_valid ( chart );
  if ( !c )
    return ( IInvalidChart );
  c->value_labels = on ? 1 : 0;
  return ( INoError );
}

IError IChartAddSeries ( IChart chart, const char *label, const double *values,
  int count, IColor color )
{
  IChartP *c = _chart_valid ( chart );
  IChartSeries *ns;
  IChartSeries *s;
  if ( !c )
    return ( IInvalidChart );
  if ( !values || count <= 0 )
    return ( IInvalidArgument );

  ns = (IChartSeries *) realloc ( c->series,
    (size_t) ( c->nseries + 1 ) * sizeof ( IChartSeries ) );
  if ( !ns )
    return ( IInvalidArgument );
  c->series = ns;
  s = &c->series[c->nseries];
  s->label = _chart_strdup ( label );
  s->values = (double *) malloc ( (size_t) count * sizeof ( double ) );
  if ( !s->values ) {
    free ( s->label );
    return ( IInvalidArgument );
  }
  memcpy ( s->values, values, (size_t) count * sizeof ( double ) );
  s->xvalues = NULL;
  s->count = count;
  s->color = color;
  c->nseries++;
  return ( INoError );
}

IError IChartAddXYSeries ( IChart chart, const char *label,
  const double *xvalues, const double *yvalues, int count, IColor color )
{
  IChartP *c = _chart_valid ( chart );
  IError ret;
  IChartSeries *s;

  if ( !c )
    return ( IInvalidChart );
  if ( !xvalues || !yvalues || count <= 0 )
    return ( IInvalidArgument );

  /* Reuse IChartAddSeries to store the y values + label + color, then attach
     a copy of the x values to the series it appended. */
  ret = IChartAddSeries ( chart, label, yvalues, count, color );
  if ( ret != INoError )
    return ( ret );
  s = &c->series[c->nseries - 1];
  s->xvalues = (double *) malloc ( (size_t) count * sizeof ( double ) );
  if ( !s->xvalues )
    return ( IInvalidArgument );
  memcpy ( s->xvalues, xvalues, (size_t) count * sizeof ( double ) );
  return ( INoError );
}

/* ----------------------------------------------------------- text helpers */

static int _chart_font_h ( IChartP *c )
{
  unsigned int h = 0;
  if ( c->font )
    IFontSize ( c->font, &h );
  return ( (int) h );
}

static int _chart_text_w ( IChartP *c, IGC gc, const char *s )
{
  unsigned int w = 0;
  if ( c->font && s )
    ITextWidth ( gc, c->font, (char *) s, (unsigned int) strlen ( s ), &w );
  return ( (int) w );
}

static void _chart_text ( IImage img, IGC gc, int x, int y, const char *s )
{
  IDrawString ( img, gc, x, y, (char *) s, (unsigned int) strlen ( s ) );
}

/* Draw a value, centered horizontally on cx with its baseline at by. The
   caller has already set the (text) foreground color. */
static void _chart_draw_value ( IImage img, IGC gc, IChartP *c, int cx, int by,
  double v )
{
  char buf[32];
  int tw;
  snprintf ( buf, sizeof ( buf ), "%g", v );
  tw = _chart_text_w ( c, gc, buf );
  _chart_text ( img, gc, cx - tw / 2, by, buf );
}

/* -------------------------------------------------------------- geometry */

static int _val_to_y ( double v, const IChartLayout *l )
{
  double t;
  if ( l->log_scale ) {
    double lo = log10 ( l->ymin ), hi = log10 ( l->ymax );
    double lv = ( v > 0.0 ) ? log10 ( v ) : lo;
    t = ( hi > lo ) ? ( lv - lo ) / ( hi - lo ) : 0.0;
  }
  else {
    double range = l->ymax - l->ymin;
    t = ( range != 0.0 ) ? ( v - l->ymin ) / range : 0.0;
  }
  return ( (int) ( l->y1 - t * ( l->y1 - l->y0 ) + 0.5 ) );
}

/* Map a scatter x value to a pixel column. */
static int _val_to_x ( double v, const IChartLayout *l )
{
  double range = l->xmax - l->xmin;
  double t = ( range != 0.0 ) ? ( v - l->xmin ) / range : 0.5;
  return ( (int) ( l->x0 + t * ( l->x1 - l->x0 ) + 0.5 ) );
}

/* Map a category index (0..n-1) to a pixel column (line/area charts). */
static int _cat_to_x ( int i, int n, const IChartLayout *l )
{
  if ( n <= 1 )
    return ( ( l->x0 + l->x1 ) / 2 );
  return ( l->x0 + i * ( l->x1 - l->x0 ) / ( n - 1 ) );
}

/* A translucent variant of a colour, for area fills. */
static IColor _chart_translucent ( IColor color, unsigned int alpha )
{
  IColorP *p = _IGetColor ( (int) color );
  if ( !p )
    return ( color );
  return ( IAllocColorAlpha ( p->red, p->green, p->blue, alpha ) );
}

static void _chart_compute_xrange ( IChartP *c, double *xmin, double *xmax )
{
  double lo = 0.0, hi = 0.0, pad;
  int found = 0, i, j;

  for ( i = 0; i < c->nseries; i++ ) {
    for ( j = 0; j < c->series[i].count; j++ ) {
      double v = c->series[i].xvalues ? c->series[i].xvalues[j] : (double) j;
      if ( !found ) {
        lo = hi = v;
        found = 1;
      }
      else {
        if ( v < lo )
          lo = v;
        if ( v > hi )
          hi = v;
      }
    }
  }
  if ( !found ) {
    lo = 0.0;
    hi = 1.0;
  }
  if ( hi <= lo )
    hi = lo + 1.0;
  pad = ( hi - lo ) * 0.05;
  *xmin = lo - pad;
  *xmax = hi + pad;
}

/* Stacked-bar range: per category, sum positive and negative values. */
static void _chart_stacked_range ( IChartP *c, double *ymin, double *ymax )
{
  int ncat = 0, i, s;
  double maxpos = 0.0, minneg = 0.0;

  for ( s = 0; s < c->nseries; s++ )
    if ( c->series[s].count > ncat )
      ncat = c->series[s].count;
  for ( i = 0; i < ncat; i++ ) {
    double p = 0.0, n = 0.0;
    for ( s = 0; s < c->nseries; s++ ) {
      if ( i < c->series[s].count ) {
        double v = c->series[s].values[i];
        if ( v >= 0.0 )
          p += v;
        else
          n += v;
      }
    }
    if ( p > maxpos )
      maxpos = p;
    if ( n < minneg )
      minneg = n;
  }
  if ( maxpos <= minneg )
    maxpos = minneg + 1.0;
  *ymin = minneg;
  *ymax = maxpos;
}

static void _chart_compute_range ( IChartP *c, double *ymin, double *ymax )
{
  double lo = 0.0, hi = 0.0, minpos = 0.0;
  int found = 0, havepos = 0, i, j;

  if ( !c->auto_range ) {
    lo = c->ymin;
    hi = c->ymax;
    if ( c->log_scale && lo <= 0.0 )
      lo = ( hi > 0.0 ) ? hi / 1000.0 : 1.0;
    *ymin = lo;
    *ymax = hi;
    return;
  }
  if ( c->stacked && c->type == ICHART_BAR ) {
    _chart_stacked_range ( c, ymin, ymax );
    return;
  }

  for ( i = 0; i < c->nseries; i++ ) {
    for ( j = 0; j < c->series[i].count; j++ ) {
      double v = c->series[i].values[j];
      if ( !found ) {
        lo = hi = v;
        found = 1;
      }
      else {
        if ( v < lo )
          lo = v;
        if ( v > hi )
          hi = v;
      }
      if ( v > 0.0 && ( !havepos || v < minpos ) ) {
        minpos = v;
        havepos = 1;
      }
    }
  }
  if ( !found ) {
    lo = 0.0;
    hi = 1.0;
  }

  if ( c->log_scale ) {
    /* Snap to enclosing powers of ten (positive data only). */
    if ( !havepos )
      minpos = 1.0;
    if ( hi <= 0.0 )
      hi = minpos * 10.0;
    lo = pow ( 10.0, floor ( log10 ( minpos ) ) );
    hi = pow ( 10.0, ceil ( log10 ( hi ) ) );
    if ( hi <= lo )
      hi = lo * 10.0;
    *ymin = lo;
    *ymax = hi;
    return;
  }

  if ( c->type == ICHART_BAR || c->type == ICHART_AREA ) {
    if ( lo > 0.0 )
      lo = 0.0; /* bars/areas need a zero baseline */
    if ( hi < 0.0 )
      hi = 0.0;
  }
  if ( hi <= lo )
    hi = lo + 1.0;
  if ( c->type == ICHART_LINE || c->type == ICHART_SCATTER ) {
    double pad = ( hi - lo ) * 0.05;
    lo -= pad;
    hi += pad;
  }
  else if ( c->type == ICHART_AREA ) {
    hi += ( hi - lo ) * 0.05; /* a little headroom above the filled area */
  }
  *ymin = lo;
  *ymax = hi;
}

/* ---------------------------------------------------------------- legend */

static void _chart_legend ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay, char **labels, IColor *colors, int count )
{
  int fh = _chart_font_h ( c );
  int sw, gap = 4, rowh, maxw = 0, boxw, bx, by, i, shown = 0;

  if ( !c->font || count <= 0 )
    return;
  for ( i = 0; i < count; i++ )
    if ( labels[i] )
      shown = 1;
  if ( !shown )
    return;

  sw = fh;
  rowh = fh + 3;
  for ( i = 0; i < count; i++ ) {
    int w = _chart_text_w ( c, gc, labels[i] );
    if ( w > maxw )
      maxw = w;
  }
  boxw = sw + gap + maxw + 8;
  bx = lay->x1 - boxw - 4;
  by = lay->y0 + 4;
  if ( bx < lay->x0 )
    bx = lay->x0;

  for ( i = 0; i < count; i++ ) {
    int ry = by + i * rowh;
    ISetForeground ( gc, colors[i] );
    IFillRectangle ( img, gc, bx + 3, ry, (unsigned int) sw, (unsigned int) sw );
    if ( labels[i] ) {
      ISetForeground ( gc, IBLACK_PIXEL );
      _chart_text ( img, gc, bx + 3 + sw + gap, ry + fh - 1, labels[i] );
    }
  }
}

/* ------------------------------------------------------------------ axes */

static void _chart_axes ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  int fh = _chart_font_h ( c );
  int i, n;

  /* Horizontal gridlines + y tick labels: powers of ten on a log axis, or 5
     evenly spaced values on a linear axis. */
  if ( lay->log_scale ) {
    int k0 = (int) floor ( log10 ( lay->ymin ) + 1e-9 );
    int k1 = (int) ceil ( log10 ( lay->ymax ) - 1e-9 );
    int k;
    if ( k1 - k0 > 12 )
      k1 = k0 + 12; /* keep the tick count sane */
    for ( k = k0; k <= k1; k++ ) {
      double val = pow ( 10.0, k );
      int y;
      if ( val < lay->ymin * 0.999 || val > lay->ymax * 1.001 )
        continue;
      y = _val_to_y ( val, lay );
      ISetForeground ( gc, _chart_grid_color () );
      IDrawLine ( img, gc, lay->x0, y, lay->x1, y );
      if ( c->font ) {
        char buf[32];
        int tw;
        snprintf ( buf, sizeof ( buf ), "%g", val );
        tw = _chart_text_w ( c, gc, buf );
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_text ( img, gc, lay->x0 - tw - 4, y + fh / 2 - 1, buf );
      }
    }
  }
  else {
    for ( i = 0; i <= 4; i++ ) {
      double val = lay->ymin + ( lay->ymax - lay->ymin ) * i / 4.0;
      int y = _val_to_y ( val, lay );
      ISetForeground ( gc, _chart_grid_color () );
      IDrawLine ( img, gc, lay->x0, y, lay->x1, y );
      if ( c->font ) {
        char buf[32];
        int tw;
        snprintf ( buf, sizeof ( buf ), "%g", val );
        tw = _chart_text_w ( c, gc, buf );
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_text ( img, gc, lay->x0 - tw - 4, y + fh / 2 - 1, buf );
      }
    }
  }

  /* Axis lines. */
  ISetForeground ( gc, IBLACK_PIXEL );
  IDrawLine ( img, gc, lay->x0, lay->y0, lay->x0, lay->y1 );
  IDrawLine ( img, gc, lay->x0, lay->y1, lay->x1, lay->y1 );

  if ( c->type == ICHART_SCATTER ) {
    /* Numeric x axis: vertical gridlines + value labels. */
    for ( i = 0; i <= 4; i++ ) {
      double val = lay->xmin + ( lay->xmax - lay->xmin ) * i / 4.0;
      int x = _val_to_x ( val, lay );
      ISetForeground ( gc, _chart_grid_color () );
      IDrawLine ( img, gc, x, lay->y0, x, lay->y1 );
      if ( c->font ) {
        char buf[32];
        int tw;
        snprintf ( buf, sizeof ( buf ), "%g", val );
        tw = _chart_text_w ( c, gc, buf );
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_text ( img, gc, x - tw / 2, lay->y1 + fh + 2, buf );
      }
    }
    /* Redraw the axis lines over the gridlines. */
    ISetForeground ( gc, IBLACK_PIXEL );
    IDrawLine ( img, gc, lay->x0, lay->y0, lay->x0, lay->y1 );
    IDrawLine ( img, gc, lay->x0, lay->y1, lay->x1, lay->y1 );
  }
  else {
    /* X category labels (line/area use point positions; bar uses slots). */
    n = c->ncategories;
    if ( c->font && n > 0 ) {
      for ( i = 0; i < n; i++ ) {
        int x, tw;
        if ( c->type == ICHART_BAR ) {
          int slot = ( lay->x1 - lay->x0 ) / n;
          x = lay->x0 + i * slot + slot / 2;
        }
        else {
          x = _cat_to_x ( i, n, lay );
        }
        tw = _chart_text_w ( c, gc, c->categories[i] );
        _chart_text ( img, gc, x - tw / 2, lay->y1 + fh + 2, c->categories[i] );
      }
    }
  }

  /* Axis titles. */
  if ( c->font && c->x_label ) {
    int tw = _chart_text_w ( c, gc, c->x_label );
    _chart_text ( img, gc, ( lay->x0 + lay->x1 ) / 2 - tw / 2, c->height - 2,
      c->x_label );
  }
  if ( c->font && c->y_label ) {
    int tw = _chart_text_w ( c, gc, c->y_label );
    IDrawStringRotated ( img, gc, fh, ( lay->y0 + lay->y1 ) / 2 + tw / 2,
      (char *) c->y_label, (unsigned int) strlen ( c->y_label ),
      ITEXT_BOTTOM_TO_TOP );
  }
}

/* -------------------------------------------------------------- plotters */

static void _chart_plot_line ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  int s, i;
  int labels = ( c->value_labels && c->font );
  for ( s = 0; s < c->nseries; s++ ) {
    IChartSeries *ser = &c->series[s];
    int n = ser->count, px = 0, py = 0;
    if ( n <= 0 )
      continue;
    for ( i = 0; i < n; i++ ) {
      int x = _cat_to_x ( i, n, lay );
      int y = _val_to_y ( ser->values[i], lay );
      ISetForeground ( gc, ser->color );
      if ( i > 0 )
        IDrawLine ( img, gc, px, py, x, y );
      IFillCircle ( img, gc, x, y, 2 );
      if ( labels ) {
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_draw_value ( img, gc, c, x, y - 4, ser->values[i] );
      }
      px = x;
      py = y;
    }
  }
}

static void _chart_plot_area ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  double base = ( lay->ymin <= 0.0 && lay->ymax >= 0.0 ) ? 0.0 : lay->ymin;
  int basey = _val_to_y ( base, lay );
  int s, i;

  for ( s = 0; s < c->nseries; s++ ) {
    IChartSeries *ser = &c->series[s];
    int n = ser->count, px = 0, py = 0;
    IColor fill;
    if ( n <= 0 )
      continue;

    /* Translucent fill under the line, built from convex per-segment quads
       (the whole area under a zig-zag line is not convex). */
    fill = _chart_translucent ( ser->color, 90 );
    ISetForeground ( gc, fill );
    ISetBlendMode ( gc, IBLEND_OVER );
    for ( i = 0; i + 1 < n; i++ ) {
      int x0 = _cat_to_x ( i, n, lay );
      int x1 = _cat_to_x ( i + 1, n, lay );
      IPoint quad[4];
      quad[0].x = x0;
      quad[0].y = basey;
      quad[1].x = x0;
      quad[1].y = _val_to_y ( ser->values[i], lay );
      quad[2].x = x1;
      quad[2].y = _val_to_y ( ser->values[i + 1], lay );
      quad[3].x = x1;
      quad[3].y = basey;
      IFillPolygon ( img, gc, quad, 4 );
    }
    ISetBlendMode ( gc, IBLEND_REPLACE );

    /* Opaque line + markers (and optional value labels) on top. */
    for ( i = 0; i < n; i++ ) {
      int x = _cat_to_x ( i, n, lay );
      int y = _val_to_y ( ser->values[i], lay );
      ISetForeground ( gc, ser->color );
      if ( i > 0 )
        IDrawLine ( img, gc, px, py, x, y );
      IFillCircle ( img, gc, x, y, 2 );
      if ( c->value_labels && c->font ) {
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_draw_value ( img, gc, c, x, y - 4, ser->values[i] );
      }
      px = x;
      py = y;
    }
  }
}

static void _chart_plot_scatter ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  int s, i;
  int labels = ( c->value_labels && c->font );
  for ( s = 0; s < c->nseries; s++ ) {
    IChartSeries *ser = &c->series[s];
    for ( i = 0; i < ser->count; i++ ) {
      double xv = ser->xvalues ? ser->xvalues[i] : (double) i;
      int x = _val_to_x ( xv, lay );
      int y = _val_to_y ( ser->values[i], lay );
      ISetForeground ( gc, ser->color );
      IFillCircle ( img, gc, x, y, 3 );
      if ( labels ) {
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_draw_value ( img, gc, c, x, y - 5, ser->values[i] );
      }
    }
  }
}

/* Stacked bars: per category, stack series above (and below) the baseline. */
static void _chart_plot_bar_stacked ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  int ncat = 0, s, i, slot, barw;

  for ( s = 0; s < c->nseries; s++ )
    if ( c->series[s].count > ncat )
      ncat = c->series[s].count;
  if ( ncat <= 0 || c->nseries <= 0 )
    return;

  slot = ( lay->x1 - lay->x0 ) / ncat;
  barw = slot * 8 / 10;
  if ( barw < 1 )
    barw = 1;

  for ( i = 0; i < ncat; i++ ) {
    double poscum = 0.0, negcum = 0.0;
    int bx = lay->x0 + i * slot + ( slot - barw ) / 2;
    for ( s = 0; s < c->nseries; s++ ) {
      double v, from, to;
      int yf, yt, top, h;
      if ( i >= c->series[s].count )
        continue;
      v = c->series[s].values[i];
      if ( v >= 0.0 ) {
        from = poscum;
        to = poscum + v;
        poscum += v;
      }
      else {
        from = negcum;
        to = negcum + v;
        negcum += v;
      }
      yf = _val_to_y ( from, lay );
      yt = _val_to_y ( to, lay );
      top = ( yt < yf ) ? yt : yf;
      h = ( yt < yf ) ? ( yf - yt ) : ( yt - yf );
      if ( h < 1 )
        h = 1;
      ISetForeground ( gc, c->series[s].color );
      IFillRectangle ( img, gc, bx, top, (unsigned int) barw,
        (unsigned int) h );
      if ( c->value_labels && c->font && h > _chart_font_h ( c ) + 2 ) {
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_draw_value ( img, gc, c, bx + barw / 2,
          top + h / 2 + _chart_font_h ( c ) / 2, v );
      }
    }
  }
}

static void _chart_plot_bar ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  int ncat = 0, s, i, slot, groupw, barw, basey;
  double base;

  if ( c->stacked ) {
    _chart_plot_bar_stacked ( img, gc, c, lay );
    return;
  }

  for ( s = 0; s < c->nseries; s++ )
    if ( c->series[s].count > ncat )
      ncat = c->series[s].count;
  if ( ncat <= 0 || c->nseries <= 0 )
    return;

  slot = ( lay->x1 - lay->x0 ) / ncat;
  groupw = slot * 8 / 10;
  if ( groupw < 1 )
    groupw = 1;
  barw = groupw / c->nseries;
  if ( barw < 1 )
    barw = 1;

  base = ( lay->ymin <= 0.0 && lay->ymax >= 0.0 ) ? 0.0 : lay->ymin;
  basey = _val_to_y ( base, lay );

  for ( s = 0; s < c->nseries; s++ ) {
    IChartSeries *ser = &c->series[s];
    for ( i = 0; i < ser->count; i++ ) {
      int slotx = lay->x0 + i * slot + ( slot - groupw ) / 2;
      int bx = slotx + s * barw;
      int by = _val_to_y ( ser->values[i], lay );
      int top = ( by < basey ) ? by : basey;
      int h = ( by < basey ) ? ( basey - by ) : ( by - basey );
      int w = ( barw > 1 ? barw - 1 : 1 );
      if ( h < 1 )
        h = 1;
      ISetForeground ( gc, ser->color );
      IFillRectangle ( img, gc, bx, top, (unsigned int) w, (unsigned int) h );
      if ( c->value_labels && c->font ) {
        ISetForeground ( gc, IBLACK_PIXEL );
        _chart_draw_value ( img, gc, c, bx + w / 2, top - 2, ser->values[i] );
      }
    }
  }
}

static void _chart_plot_pie ( IImage img, IGC gc, IChartP *c,
  const IChartLayout *lay )
{
  IChartSeries *ser;
  double total = 0.0, a0 = 0.0;
  int i, cx, cy, r, side;

  if ( c->nseries <= 0 )
    return;
  ser = &c->series[0];
  for ( i = 0; i < ser->count; i++ )
    if ( ser->values[i] > 0.0 )
      total += ser->values[i];
  if ( total <= 0.0 )
    return;

  cx = ( lay->x0 + lay->x1 ) / 2;
  cy = ( lay->y0 + lay->y1 ) / 2;
  side = ( ( lay->x1 - lay->x0 ) < ( lay->y1 - lay->y0 ) )
           ? ( lay->x1 - lay->x0 )
           : ( lay->y1 - lay->y0 );
  r = side / 2 - 2;
  if ( r < 1 )
    r = 1;

  for ( i = 0; i < ser->count; i++ ) {
    double a1;
    if ( ser->values[i] <= 0.0 )
      continue;
    a1 = a0 + ser->values[i] / total * 360.0;
    ISetForeground ( gc, _chart_palette ( i ) );
    IFillArc ( img, gc, cx, cy, r, r, a0, a1 );
    if ( c->value_labels && c->font ) {
      /* Place the label at the slice's angular midpoint, partway out from the
         centre. Angles are negated for the y-down coordinate system, matching
         IArcProperties / IFillArc. */
      double mid = ( a0 + a1 ) / 2.0;
      double ang = ( 2.0 * PI / 360.0 ) * ( 360.0 - mid );
      int lx = cx + (int) ( r * 0.6 * cos ( ang ) );
      int ly = cy + (int) ( r * 0.6 * sin ( ang ) );
      ISetForeground ( gc, IBLACK_PIXEL );
      _chart_draw_value ( img, gc, c, lx, ly + _chart_font_h ( c ) / 2,
        ser->values[i] );
    }
    a0 = a1;
  }
}

/* --------------------------------------------------------------- render */

IImage IChartRender ( IChart chart )
{
  IChartP *c = _chart_valid ( chart );
  IImage img;
  IGC gc;
  IChartLayout lay;
  int fh, left, right, top, bottom;

  if ( !c )
    return ( NULL );

  img = ICreateImage ( (unsigned int) c->width, (unsigned int) c->height,
    IOPTION_NONE );
  if ( !img )
    return ( NULL );
  gc = ICreateGC ();
  if ( !gc ) {
    IFreeImage ( img );
    return ( NULL );
  }
  ISetAntiAlias ( gc, 1 );
  /* IDrawString() draws with the GC's font; mirror the chart's font onto it. */
  if ( c->font )
    ISetFont ( gc, c->font );

  ISetForeground ( gc, c->background );
  IFillRectangle ( img, gc, 0, 0, (unsigned int) c->width,
    (unsigned int) c->height );

  fh = _chart_font_h ( c );

  left = 15;
  right = 15;
  top = 10;
  bottom = 10;
  if ( c->title && c->font )
    top += fh + 6;
  if ( c->type != ICHART_PIE ) {
    left = c->font ? 50 : 15;
    bottom = 12;
    if ( c->font )
      bottom += fh + 4; /* x category labels */
    if ( c->x_label && c->font )
      bottom += fh + 2;
    if ( c->y_label && c->font )
      left += fh + 2;
  }

  lay.x0 = left;
  lay.y0 = top;
  lay.x1 = c->width - 1 - right;
  lay.y1 = c->height - 1 - bottom;
  if ( lay.x1 <= lay.x0 )
    lay.x1 = lay.x0 + 1;
  if ( lay.y1 <= lay.y0 )
    lay.y1 = lay.y0 + 1;
  lay.ymin = 0.0;
  lay.ymax = 1.0;
  lay.xmin = 0.0;
  lay.xmax = 1.0;
  /* Stacking in log space is meaningless, so a stacked bar stays linear. */
  lay.log_scale =
    ( c->log_scale && !( c->stacked && c->type == ICHART_BAR ) ) ? 1 : 0;

  if ( c->title && c->font ) {
    int tw = _chart_text_w ( c, gc, c->title );
    ISetForeground ( gc, IBLACK_PIXEL );
    _chart_text ( img, gc, ( c->width - tw ) / 2, fh, c->title );
  }

  if ( c->type == ICHART_PIE ) {
    _chart_plot_pie ( img, gc, c, &lay );
    if ( c->ncategories > 0 ) {
      IColor *cols =
        (IColor *) malloc ( (size_t) c->ncategories * sizeof ( IColor ) );
      if ( cols ) {
        int i;
        for ( i = 0; i < c->ncategories; i++ )
          cols[i] = _chart_palette ( i );
        _chart_legend ( img, gc, c, &lay, c->categories, cols,
          c->ncategories );
        free ( cols );
      }
    }
  }
  else {
    _chart_compute_range ( c, &lay.ymin, &lay.ymax );
    if ( c->type == ICHART_SCATTER )
      _chart_compute_xrange ( c, &lay.xmin, &lay.xmax );
    _chart_axes ( img, gc, c, &lay );
    if ( c->type == ICHART_LINE )
      _chart_plot_line ( img, gc, c, &lay );
    else if ( c->type == ICHART_BAR )
      _chart_plot_bar ( img, gc, c, &lay );
    else if ( c->type == ICHART_AREA )
      _chart_plot_area ( img, gc, c, &lay );
    else /* ICHART_SCATTER */
      _chart_plot_scatter ( img, gc, c, &lay );
    if ( c->nseries > 0 ) {
      char **labels =
        (char **) malloc ( (size_t) c->nseries * sizeof ( char * ) );
      IColor *cols =
        (IColor *) malloc ( (size_t) c->nseries * sizeof ( IColor ) );
      if ( labels && cols ) {
        int i;
        for ( i = 0; i < c->nseries; i++ ) {
          labels[i] = c->series[i].label;
          cols[i] = c->series[i].color;
        }
        _chart_legend ( img, gc, c, &lay, labels, cols, c->nseries );
      }
      free ( labels );
      free ( cols );
    }
  }

  IFreeGC ( gc );
  return ( img );
}
