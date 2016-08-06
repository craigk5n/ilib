/*
 * IDrawRec.c
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


IError IDrawRectangle ( image, gc, x, y, w, h )
IImage image;
IGC gc;
int x;
int y;
unsigned int w;
unsigned int h;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  IDrawLine ( image, gc, x, y, x + w, y );
  IDrawLine ( image, gc, x, y, x, y + h );
  IDrawLine ( image, gc, x, y + h, x + w, y + h );
  IDrawLine ( image, gc, x + w, y, x + w, y + h );

  return ( INoError );
}



