/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IDrawLine.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"


#define ON_OFF_PIXELS 3.0

static double fpart ( double x )
{
  return ( x - floor ( x ) );
}
static double rfpart ( double x )
{
  return ( 1.0 - fpart ( x ) );
}

/* Composite the foreground at (px,py) with fractional coverage c (0..1). */
static void aa_plot ( IImageP *img, IGCP *gc, int px, int py, double c )
{
  long cover;
  if ( c <= 0.0 )
    return;
  cover = (long) ( c * 255.0 + 0.5 );
  if ( cover <= 0 )
    return;
  if ( cover > 255 )
    cover = 255;
  _IBlendPoint ( img, gc, px, py, (unsigned int) cover );
}

/* Xiaolin Wu's anti-aliased line. Endpoints are integers here, but the
   algorithm is written in floating point so the coverage at the line edges is
   smooth. Used for width-1 solid lines when GC anti-aliasing is on. */
static void draw_line_aa ( IImageP *img, IGCP *gc, int ix0, int iy0, int ix1,
  int iy1 )
{
  double x0 = ix0, y0 = iy0, x1 = ix1, y1 = iy1;
  int steep = fabs ( y1 - y0 ) > fabs ( x1 - x0 );
  double dx, dy, gradient, xend, yend, xgap, intery, t;
  int xpxl1, xpxl2, x;

  if ( steep ) {
    t = x0;
    x0 = y0;
    y0 = t;
    t = x1;
    x1 = y1;
    y1 = t;
  }
  if ( x0 > x1 ) {
    t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }

  dx = x1 - x0;
  dy = y1 - y0;
  gradient = ( dx == 0.0 ) ? 1.0 : dy / dx;

  /* first endpoint */
  xend = floor ( x0 + 0.5 );
  yend = y0 + gradient * ( xend - x0 );
  xgap = rfpart ( x0 + 0.5 );
  xpxl1 = (int) xend;
  if ( steep ) {
    aa_plot ( img, gc, (int) floor ( yend ), xpxl1, rfpart ( yend ) * xgap );
    aa_plot ( img, gc, (int) floor ( yend ) + 1, xpxl1, fpart ( yend ) * xgap );
  }
  else {
    aa_plot ( img, gc, xpxl1, (int) floor ( yend ), rfpart ( yend ) * xgap );
    aa_plot ( img, gc, xpxl1, (int) floor ( yend ) + 1, fpart ( yend ) * xgap );
  }
  intery = yend + gradient;

  /* second endpoint */
  xend = floor ( x1 + 0.5 );
  yend = y1 + gradient * ( xend - x1 );
  xgap = fpart ( x1 + 0.5 );
  xpxl2 = (int) xend;
  if ( steep ) {
    aa_plot ( img, gc, (int) floor ( yend ), xpxl2, rfpart ( yend ) * xgap );
    aa_plot ( img, gc, (int) floor ( yend ) + 1, xpxl2, fpart ( yend ) * xgap );
  }
  else {
    aa_plot ( img, gc, xpxl2, (int) floor ( yend ), rfpart ( yend ) * xgap );
    aa_plot ( img, gc, xpxl2, (int) floor ( yend ) + 1, fpart ( yend ) * xgap );
  }

  /* main span */
  for ( x = xpxl1 + 1; x < xpxl2; x++ ) {
    if ( steep ) {
      aa_plot ( img, gc, (int) floor ( intery ), x, rfpart ( intery ) );
      aa_plot ( img, gc, (int) floor ( intery ) + 1, x, fpart ( intery ) );
    }
    else {
      aa_plot ( img, gc, x, (int) floor ( intery ), rfpart ( intery ) );
      aa_plot ( img, gc, x, (int) floor ( intery ) + 1, fpart ( intery ) );
    }
    intery += gradient;
  }
}

IError IDrawLine ( IImage image, IGC gc, int x1, int y1, int x2, int y2 )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int myx, myy;
  double slope = 0.0;
  double myslope;
  double curx, cury;
  int done = 0;
  int temp;
  double draw_count = 0.0;
  double on_off_size = 0.0;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* Anti-aliased path for thin solid lines (thick/dashed lines still use the
     integer rasterizer below). */
  if ( gcp->aa && gcp->line_width <= 1 && gcp->line_style == ILINE_SOLID ) {
    draw_line_aa ( imagep, gcp, x1, y1, x2, y2 );
    return ( INoError );
  }

  /* x2 should always be greater than x1 */
  if ( x2 < x1 ) {
    temp = x2;
    x2 = x1;
    x1 = temp;
    temp = y2;
    y2 = y1;
    y1 = temp;
  }

  /* remember, our coordinate system is reversed for y */
  if ( x1 == x2 ) {
    if ( y2 < y1 ) {
      temp = x2;
      x2 = x1;
      x1 = temp;
      temp = y2;
      y2 = y1;
      y1 = temp;
    }
  }
  else
    slope = -( (double) y2 - (double) y1 ) / ( (double) x1 - (double) x2 );
  curx = (double) x1;
  cury = (double) y1;

  if ( gcp->line_width <= 1 )
    _IDrawPoint ( imagep, gcp, x1, y1 );

  /* handle dashes */
  if ( gcp->line_style == ILINE_ON_OFF_DASH ) {
    if ( x1 == x2 || fabs ( slope ) < 0.1 || fabs ( slope ) > 10 ) {
      on_off_size = ON_OFF_PIXELS;
    }
    else {
      myslope = fabs ( slope );
      if ( myslope > 1.0 )
        myslope = 1.0 / myslope;
      /* myslope now between 0 and 1.0 */
      on_off_size = ON_OFF_PIXELS + ( 0.41 * myslope );
    }
  }

  while ( !done ) {
    if ( x1 == x2 ) {
      if ( cury >= (double) y2 )
        done = 1;
      else
        cury += 1.0;
    }
    else if ( slope >= 1.0 ) {
      if ( cury >= (double) y2 )
        done = 1;
      else {
        cury += 1.0;
        curx += ( 1.0 / slope );
      }
    }
    else if ( slope < -1.0 ) {
      if ( cury <= (double) y2 )
        done = 1;
      else {
        cury -= 1.0;
        curx -= ( 1.0 / slope );
      }
    }
    else if ( slope >= 0.0 ) {
      if ( curx >= (double) x2 )
        done = 1;
      else {
        curx += 1.0;
        cury += slope;
      }
    }
    else if ( slope < 0.0 ) {
      if ( curx >= (double) x2 )
        done = 1;
      else {
        curx += 1.0;
        cury += slope;
      }
    }

    if ( gcp->line_style == ILINE_ON_OFF_DASH ) {
      draw_count += 1.0;
      if ( ( (int) ( floor ( draw_count / on_off_size ) ) % 2 ) == 1 )
        continue;
    }

    if ( !done ) {
      myx = (int) curx;
      myy = (int) cury;
      switch ( gcp->line_width ) {
      default:
      case 0:
      case 1:
        _IDrawPoint ( imagep, gcp, myx, myy );
        break;
      case 2:
        _IDrawPoint ( imagep, gcp, myx, myy );
        _IDrawPoint ( imagep, gcp, myx - 1, myy );
        _IDrawPoint ( imagep, gcp, myx - 1, myy - 1 );
        _IDrawPoint ( imagep, gcp, myx, myy - 1 );
        break;
      case 3:
        _IDrawPoint ( imagep, gcp, myx, myy );
        _IDrawPoint ( imagep, gcp, myx - 1, myy );
        _IDrawPoint ( imagep, gcp, myx + 1, myy - 1 );
        _IDrawPoint ( imagep, gcp, myx, myy - 1 );
        _IDrawPoint ( imagep, gcp, myx, myy + 1 );
        break;
      }
    }
  }

  return ( INoError );
}
