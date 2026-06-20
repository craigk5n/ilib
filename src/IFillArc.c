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


/* Anti-aliased filled ellipse via NxN supersampled coverage against the true
   ellipse equation. Going through the integer-vertex polygon (IFillPolygon)
   loses the sub-pixel edge, so a full ellipse/circle is filled directly here. */
#define IFILLARC_AA_SS 4
static void fill_ellipse_aa ( IImageP *img, IGCP *gc, int cx, int cy, int rx,
  int ry )
{
  int x, y, sx, sy, cnt, x0, y0, x1, y1;
  double rx2, ry2;

  if ( rx < 1 || ry < 1 )
    return;
  rx2 = (double) rx * rx;
  ry2 = (double) ry * ry;

  x0 = cx - rx;
  y0 = cy - ry;
  x1 = cx + rx;
  y1 = cy + ry;
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
      cnt = 0;
      for ( sy = 0; sy < IFILLARC_AA_SS; sy++ ) {
        for ( sx = 0; sx < IFILLARC_AA_SS; sx++ ) {
          double dx = ( x + ( sx + 0.5 ) / IFILLARC_AA_SS ) - cx;
          double dy = ( y + ( sy + 0.5 ) / IFILLARC_AA_SS ) - cy;
          if ( dx * dx / rx2 + dy * dy / ry2 <= 1.0 )
            cnt++;
        }
      }
      if ( cnt )
        _IBlendPoint ( img, gc, x, y,
          (unsigned int) ( cnt * 255 / ( IFILLARC_AA_SS * IFILLARC_AA_SS ) ) );
    }
  }
}


IError IFillArc ( IImage image, IGC gc, int x, int y, int r1, int r2, double a1, double a2 )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int N, loop;
  double a, da;
  IPoint *points;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* Anti-aliased filled ellipse/circle (full sweep). Partial arcs (pie wedges)
     fall through to the polygon fill, which is anti-aliased on its straight
     edges when GC anti-aliasing is on. */
  if ( gcp->aa && fabs ( a2 - a1 ) >= 359.9 ) {
    fill_ellipse_aa ( imagep, gcp, x, y, r1, r2 );
    return ( INoError );
  }

  /* because our y is upside down, make all angles their negative */
  a1 = 360 - a1;
  a2 = 360 - a2;

  N = (int) fabs ( a2 - a1 ) + 9;
  a = a1 * 2.0 * PI / 360.0;
  da = ( a2 - a1 ) * ( 2.0 * PI / 360.0 ) / ( N - 1 );
  points = (IPoint *) malloc ( sizeof ( IPoint ) * ( N + 1 ) );
  for ( loop = 0; loop < N; loop++ ) {
    points[loop].x = x + (int) ( r1 * cos ( a + loop * da ) );
    points[loop].y = y + (int) ( r2 * sin ( a + loop * da ) );
  }

  /* if we're not drawing a circle, add in the center point */
  if ( a2 - a1 < 359.9 ) {
    points[N].x = x;
    points[N].y = y;
    IFillPolygon ( image, gc, points, N + 1 );
  }
  else {
    IFillPolygon ( image, gc, points, N );
  }

  free ( points );

  return ( INoError );
}
