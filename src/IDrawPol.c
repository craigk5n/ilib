/*
 * IDrawPol.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
*	22-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
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



IError IDrawPolygon ( image, gc, points, npoints )
IImage image;
IGC gc;
IPoint *points;
int npoints;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  int loop;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  for ( loop = 1; loop < npoints; loop++ ) {
    IDrawLine ( image, gc, points[loop-1].x, points[loop-1].y,
      points[loop].x, points[loop].y );
  }

  return ( INoError );
}



