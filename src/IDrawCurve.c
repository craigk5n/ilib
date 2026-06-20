/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IDrawCurve.c
 *
 * Image library
 *
 * Description:
 *	Smooth curves: cubic Bezier paths and Catmull-Rom splines. Both are
 *	flattened to short line segments and drawn with IDrawLine(), so they
 *	inherit the graphics context's line style and anti-aliasing.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"

static double pt_dist ( IPoint a, IPoint b )
{
  double dx = (double) a.x - b.x;
  double dy = (double) a.y - b.y;
  return ( sqrt ( dx * dx + dy * dy ) );
}

/* Number of flattening samples for a segment, from its (approximate) length:
   ~1 sample every 3 pixels, bounded so degenerate input stays cheap. */
static int curve_samples ( double length )
{
  int n = (int) ( length / 3.0 );
  if ( n < 4 )
    n = 4;
  if ( n > 4096 )
    n = 4096;
  return ( n );
}

/* Draw a chain of cubic Bezier segments. points[0] is the start; every
   following group of three points is (control1, control2, end) of one cubic,
   so npoints must be 4, 7, 10, ... (1 + 3k). */
IError IDrawBezier ( IImage image, IGC gc, IPoint *points, int npoints )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int seg, i;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( npoints < 4 || ( ( npoints - 1 ) % 3 ) != 0 )
    return ( IInvalidArgument );

  for ( seg = 0; seg + 3 < npoints; seg += 3 ) {
    IPoint p0 = points[seg], p1 = points[seg + 1];
    IPoint p2 = points[seg + 2], p3 = points[seg + 3];
    int n = curve_samples ( pt_dist ( p0, p1 ) + pt_dist ( p1, p2 ) +
                            pt_dist ( p2, p3 ) );
    int prevx = p0.x, prevy = p0.y;
    for ( i = 1; i <= n; i++ ) {
      double t = (double) i / n, mt = 1.0 - t;
      double b0 = mt * mt * mt, b1 = 3 * mt * mt * t;
      double b2 = 3 * mt * t * t, b3 = t * t * t;
      int x = (int) ( b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x + 0.5 );
      int y = (int) ( b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y + 0.5 );
      IDrawLine ( image, gc, prevx, prevy, x, y );
      prevx = x;
      prevy = y;
    }
  }

  return ( INoError );
}

/* Draw a Catmull-Rom spline passing through all the given points (the curve
   interpolates each point). Endpoints are clamped (the first/last point is
   duplicated). npoints >= 2; two points draw a straight line. */
IError IDrawSpline ( IImage image, IGC gc, IPoint *points, int npoints )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int seg, i;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( npoints < 2 )
    return ( IInvalidArgument );

  for ( seg = 0; seg < npoints - 1; seg++ ) {
    IPoint p0 = points[seg > 0 ? seg - 1 : 0];
    IPoint p1 = points[seg];
    IPoint p2 = points[seg + 1];
    IPoint p3 = points[seg + 2 < npoints ? seg + 2 : npoints - 1];
    int n = curve_samples ( pt_dist ( p1, p2 ) * 1.5 );
    int prevx = p1.x, prevy = p1.y;
    for ( i = 1; i <= n; i++ ) {
      double t = (double) i / n, t2 = t * t, t3 = t2 * t;
      double x = 0.5 * ( ( 2.0 * p1.x ) + ( -p0.x + p2.x ) * t +
                         ( 2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x ) * t2 +
                         ( -p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x ) * t3 );
      double y = 0.5 * ( ( 2.0 * p1.y ) + ( -p0.y + p2.y ) * t +
                         ( 2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y ) * t2 +
                         ( -p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y ) * t3 );
      int ix = (int) ( x + 0.5 ), iy = (int) ( y + 0.5 );
      IDrawLine ( image, gc, prevx, prevy, ix, iy );
      prevx = ix;
      prevy = iy;
    }
  }

  return ( INoError );
}
