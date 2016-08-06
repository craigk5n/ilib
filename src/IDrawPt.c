/*
 * IDrawPt.c
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


IError IDrawPoint ( image, gc, x, y )
IImage image;
IGC gc;
int x;
int y;
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

  _IDrawPoint ( imagep, gcp, x, y )

  return ( INoError );
}



