/*
 * IFlood.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	25-Oct-04	Craig Knudsen	cknudsen@cknudsen.com
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



IError IFloodFill ( image, gc, x, y )
IImage image;
IGC gc;
int x;
int y;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  int fillL, fillR, i;
  int in_line = 1;
  IColorP color;
  IColorP origColor; /* color we are replacing with flood fill */

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* Get the color we are replacing with the flood fill */
  _IGetPointColor ( imagep, x, y, origColor );

  /* find left side, filling along the way */
  fillL = fillR = x;
  while ( in_line ) {
    _ISetPoint ( imagep, gcp, fillL, y );
    fillL--;
    _IGetPointColor ( imagep, fillL, y, color );
    in_line = ( fillL < 0 ) ? 0 : _IColorsMatch ( color, origColor );
  }
  fillL++;

  /* find right side, filling along the way */
  in_line = 1;
  while ( in_line ) {
    _ISetPoint ( imagep, gcp, fillR, y );
    fillR++;
    _IGetPointColor ( imagep, fillR, y, color );
    in_line = ( fillR >= imagep->height ) ? 0 : _IColorsMatch ( color, origColor );
  }
  fillR--;

  /* search top and bottom */
  for ( i = fillL; i <= fillR; i++ ) {
    _IGetPointColor ( imagep, i, y - 1, color );
    if ( y > 0 && _IColorsMatch ( color, origColor ) )
      IFloodFill ( image, gc, i, y - 1 );

    _IGetPointColor ( imagep, i, y + 1, color );
    if ( y < imagep->height && _IColorsMatch ( color, origColor ) )
      IFloodFill ( image, gc, i, y + 1 );
  }

  return ( INoError );
}



