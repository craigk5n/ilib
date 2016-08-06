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
#include <memory.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"



IError IFillArc ( image, gc, x, y, r1, r2, a1, a2 )
IImage image;
IGC gc;
int x;
int y;
int r1;
int r2;
double a1; /* arc 1 (in degrees) */
double a2; /* arc 2 (in degrees) */
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  int N, loop;
  double a, da;
  IPoint *points;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* because our y is upside down, make all angles their negative */
  a1 = 360 - a1;
  a2 = 360 - a2;

  N = (int) fabs ( a2 - a1 ) + 9;
  a = a1 * 2.0 * PI / 360.0;
  da = ( a2 - a1 ) * ( 2.0 * PI / 360.0 ) / ( N - 1 );
  points = (IPoint *) malloc ( sizeof ( IPoint ) * ( N + 1 ) );
  for ( loop = 0; loop < N ; loop++ ) {
    points[loop].x = x + (int)( r1 * cos ( a + loop * da ) );
    points[loop].y = y + (int)( r2 * sin ( a + loop * da ) );
  }

  /* if we're not drawing a circle, add in the center point */
  if ( a2 - a1 < 359.9 ) {
    points[N].x = x;
    points[N].y = y;
    IFillPolygon ( image, gc, points, N + 1 );
  } else {
    IFillPolygon ( image, gc, points, N );
  }

  free ( points );

  return ( INoError );
}



