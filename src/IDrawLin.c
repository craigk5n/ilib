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
#include <memory.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"


#define ON_OFF_PIXELS	3.0

IError IDrawLine ( image, gc, x1, y1, x2, y2 )
IImage image;
IGC gc;
int x1;
int y1;
int x2;
int y2;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  int myx, myy;
  double slope;
  double myslope;
  double curx, cury;
  int done = 0;
  int temp;
  double draw_count = 0.0;
  double on_off_size = 0.0;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

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
    slope = - ( (double) y2 - (double) y1 ) / ( (double) x1 - (double) x2 );
  curx = (double) x1;
  cury = (double) y1;

  if ( gcp->line_width <= 1 )
    _IDrawPoint ( imagep, gcp, x1, y1 );

  /* handle dashes */
  if ( gcp->line_style == ILINE_ON_OFF_DASH ) {
    if ( x1 == x2 || abs ( slope ) < 0.1 || abs ( slope ) > 10 ) {
      on_off_size = ON_OFF_PIXELS;
    }
    else {
      myslope = abs ( slope );
      if ( myslope > 1.0 )
        myslope = 1.0 / myslope;
      /* myslope now between 0 and 1.0 */
      on_off_size = ON_OFF_PIXELS + ( 0.41 * myslope );
    }
  }

  while ( ! done ) {
    if ( x1 == x2 ) {
      if ( cury >= (double)y2 )
        done = 1;
      else
        cury += 1.0;
    }
    else if ( slope >= 1.0 ) {
      if ( cury >= (double)y2 )
        done = 1;
      else {
        cury += 1.0;
        curx += ( 1.0 / slope );
      }
    }
    else if ( slope < -1.0 ) {
      if ( cury <= (double)y2 )
        done = 1;
      else {
        cury -= 1.0;
        curx -= ( 1.0 / slope );
      }
    }
    else if ( slope >= 0.0 ) {
      if ( curx >= (double)x2 )
        done = 1;
      else {
        curx += 1.0;
        cury += slope;
      }
    }
    else if ( slope < 0.0 ) {
      if ( curx >= (double)x2 )
        done = 1;
      else {
        curx += 1.0;
        cury += slope;
      }
    }

    if ( gcp->line_style == ILINE_ON_OFF_DASH ) {
      draw_count += 1.0;
      if ( ( (int)( floor ( draw_count / on_off_size ) ) % 2 ) == 1 )
        continue;
    }

    if ( ! done ) {
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



