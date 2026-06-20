/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IDrawArc.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	28-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IDrawEnclosedArc()
 *	19-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
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


/* Blend the foreground at the 8 symmetric octant points of (dx,dy) about the
   circle center, each with fractional coverage cov. */
static void circ_plot8 ( IImageP *img, IGCP *gc, int cx, int cy, int dx, int dy,
  double cov )
{
  long c = (long) ( cov * 255.0 + 0.5 );
  if ( c <= 0 )
    return;
  if ( c > 255 )
    c = 255;
  _IBlendPoint ( img, gc, cx + dx, cy + dy, (unsigned int) c );
  _IBlendPoint ( img, gc, cx - dx, cy + dy, (unsigned int) c );
  _IBlendPoint ( img, gc, cx + dx, cy - dy, (unsigned int) c );
  _IBlendPoint ( img, gc, cx - dx, cy - dy, (unsigned int) c );
  /* The (dy,dx) reflection coincides with the above on the 45-degree
     diagonal; skip it there so those pixels are not blended twice. */
  if ( dx != dy ) {
    _IBlendPoint ( img, gc, cx + dy, cy + dx, (unsigned int) c );
    _IBlendPoint ( img, gc, cx - dy, cy + dx, (unsigned int) c );
    _IBlendPoint ( img, gc, cx + dy, cy - dx, (unsigned int) c );
    _IBlendPoint ( img, gc, cx - dy, cy - dx, (unsigned int) c );
  }
}

/* Xiaolin Wu's anti-aliased circle outline (radius r about (cx,cy)). */
static void aa_circle ( IImageP *img, IGCP *gc, int cx, int cy, int r )
{
  int xx, yi;
  double yy, frac;

  if ( r < 1 ) {
    _IBlendPoint ( img, gc, cx, cy, 255 );
    return;
  }
  for ( xx = 0;; xx++ ) {
    yy = sqrt ( (double) r * r - (double) xx * xx );
    yi = (int) floor ( yy );
    if ( xx > yi ) /* past 45 degrees; the octant symmetry covers the rest */
      break;
    frac = yy - yi;
    circ_plot8 ( img, gc, cx, cy, xx, yi, 1.0 - frac );
    circ_plot8 ( img, gc, cx, cy, xx, yi + 1, frac );
  }
}

/* Anti-aliased ellipse outline (radii rx,ry about (cx,cy)). Coverage is
   estimated from the implicit ellipse function f = (dx/rx)^2 + (dy/ry)^2 - 1,
   whose value divided by its gradient magnitude approximates the distance to
   the curve in pixels; a 1px-wide line is 1 - that distance. */
static void aa_ellipse_outline ( IImageP *img, IGCP *gc, int cx, int cy, int rx,
  int ry )
{
  int x, y, x0, y0, x1, y1;
  double rx2, ry2;

  if ( rx < 1 || ry < 1 )
    return;
  rx2 = (double) rx * rx;
  ry2 = (double) ry * ry;

  x0 = cx - rx - 1;
  y0 = cy - ry - 1;
  x1 = cx + rx + 1;
  y1 = cy + ry + 1;
  if ( x0 < 0 )
    x0 = 0;
  if ( y0 < 0 )
    y0 = 0;
  if ( x1 >= img->width )
    x1 = img->width - 1;
  if ( y1 >= img->height )
    y1 = img->height - 1;

  for ( y = y0; y <= y1; y++ ) {
    for ( x = x0; x <= x1; x++ ) {
      double dx = x - cx;
      double dy = y - cy;
      double f = dx * dx / rx2 + dy * dy / ry2 - 1.0;
      double gx = 2.0 * dx / rx2;
      double gy = 2.0 * dy / ry2;
      double grad = sqrt ( gx * gx + gy * gy );
      double cov;
      if ( grad < 1e-9 )
        continue;
      cov = 1.0 - fabs ( f ) / grad;
      if ( cov <= 0.0 )
        continue;
      if ( cov > 1.0 )
        cov = 1.0;
      _IBlendPoint ( img, gc, x, y, (unsigned int) ( cov * 255.0 + 0.5 ) );
    }
  }
}


IError IDrawArc ( IImage image, IGC gc, int x, int y, int r1, int r2, double a1, double a2 )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int myx, myy, lastx = 0, lasty = 0, N, loop;
  double a, da;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* Anti-aliased full circle/ellipse (full 360 sweep). Partial arcs fall
     through to the segment approximation, which is itself anti-aliased when GC
     anti-aliasing is on (each segment is an AA line). The Wu circle is used for
     r1==r2; ellipses use the implicit-distance outline. */
  if ( gcp->aa && fabs ( a2 - a1 ) >= 360.0 ) {
    if ( r1 == r2 )
      aa_circle ( imagep, gcp, x, y, r1 );
    else
      aa_ellipse_outline ( imagep, gcp, x, y, r1, r2 );
    return ( INoError );
  }

  /* because our y is upside down, make all angles their negative */
  a1 = 360 - a1;
  a2 = 360 - a2;

  N = (int) fabs ( a2 - a1 ) + 8;
  a = a1 * 2.0 * PI / 360.0;
  da = ( a2 - a1 ) * ( 2.0 * PI / 360.0 ) / ( N - 1 );
  for ( loop = 0; loop < N; loop++ ) {
    myx = x + (int) ( r1 * cos ( a + loop * da ) );
    myy = y + (int) ( r2 * sin ( a + loop * da ) );
    if ( loop )
      IDrawLine ( image, gc, lastx, lasty, myx, myy );
    lastx = myx;
    lasty = myy;
  }

  return ( INoError );
}


/*
** Draw an arc and connect it to the center point.
*/
IError IDrawEnclosedArc ( IImage image, IGC gc, int x, int y, int r1, int r2, double a1, double a2 )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int myx, myy, lastx = 0, lasty = 0, N, loop;
  double a, da;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* because our y is upside down, make all angles their negative */
  a1 = 360 - a1;
  a2 = 360 - a2;

  N = (int) fabs ( a2 - a1 ) + 8;
  a = a1 * 2.0 * PI / 360.0;
  da = ( a2 - a1 ) * ( 2.0 * PI / 360.0 ) / ( N - 1 );
  for ( loop = 0; loop < N; loop++ ) {
    myx = x + (int) ( r1 * cos ( a + loop * da ) );
    myy = y + (int) ( r2 * sin ( a + loop * da ) );
    if ( loop )
      IDrawLine ( image, gc, lastx, lasty, myx, myy );
    if ( loop == N - 1 || loop == 0 )
      IDrawLine ( image, gc, x, y, myx, myy );
    lastx = myx;
    lasty = myy;
  }

  return ( INoError );
}
