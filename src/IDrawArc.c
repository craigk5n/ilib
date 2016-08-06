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
#include <memory.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"



IError IDrawArc ( image, gc, x, y, r1, r2, a1, a2 )
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
  int myx, myy, lastx, lasty, N, loop;
  double a, da;

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

  N = (int) fabs ( a2 - a1 ) + 8;
  a = a1 * 2.0 * PI / 360.0;
  da = ( a2 - a1 ) * ( 2.0 * PI / 360.0 ) / ( N - 1 );
  for ( loop = 0; loop < N ; loop++ ) {
    myx = x + (int)( r1 * cos ( a + loop * da ) );
    myy = y + (int)( r2 * sin ( a + loop * da ) );
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
IError IDrawEnclosedArc ( image, gc, x, y, r1, r2, a1, a2 )
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
  int myx, myy, lastx, lasty, N, loop;
  double a, da;

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

  N = (int) fabs ( a2 - a1 ) + 8;
  a = a1 * 2.0 * PI / 360.0;
  da = ( a2 - a1 ) * ( 2.0 * PI / 360.0 ) / ( N - 1 );
  for ( loop = 0; loop < N ; loop++ ) {
    myx = x + (int)( r1 * cos ( a + loop * da ) );
    myy = y + (int)( r2 * sin ( a + loop * da ) );
    if ( loop )
      IDrawLine ( image, gc, lastx, lasty, myx, myy );
    if ( loop == N - 1 || loop == 0 )
      IDrawLine ( image, gc, x, y, myx, myy );
    lastx = myx;
    lasty = myy;
  }

  return ( INoError );
}




