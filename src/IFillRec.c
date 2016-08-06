/*
 * IFillRec.c
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

#include "Ilib.h"
#include "IlibP.h"


IError IFillRectangle ( image, gc, x, y, w, h )
IImage image;
IGC gc;
int x;
int y;
unsigned int w;
unsigned int h;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  int row, col;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  for ( row = y; row < y + h && row < imagep->height; row++ ) {
    for ( col = x; col < x + w && col < imagep->width; col++ ) {
      if ( row >= 0 && col >= 0 ) {
        _ISetPoint ( imagep, gcp, col, row );
      }
    }
  }

  return ( INoError );
}



