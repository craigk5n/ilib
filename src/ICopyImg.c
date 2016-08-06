/*
 * ICopyImg.c
 *
 * Image library
 *
 * Description:
 *	Copy an area of an image to another image.
 *
 * History:
 *	15-Aug-01	Craig Knudsen	cknudsen@cknudsen.com
 *			Fixed bug in ICopyImageScaled
 *			(thanks Gal Steinitz for this fix)
 *	23-Jul-99	Craig Knudsen   cknudsen@cknudsen.com
 *			Added ICopyImageScaled
 *	11-Nov-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Allow transparent values to not be copied.
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


IError ICopyImage ( source, dest, gc, src_x, src_y, width, height,
  dest_x, dest_y )
IImage source, dest;
IGC gc;
int src_x, src_y;
unsigned int width, height;
int dest_x, dest_y;
{
  IImageP *i1 = (IImageP *) source;
  IImageP *i2 = (IImageP *) dest;
  IGCP *gcp = (IGCP *) gc;
  int row, col, x, y;
  IColor color;
  IColorP *colorp;
  IColorP *save;
  unsigned char *ptr;

  if ( i1 ) {
    if ( i1->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( i2 ) {
    if ( i2->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }
  else
    return ( IInvalidGC );

  save = gcp->foreground;

  color = IAllocColor ( 0, 0, 0 );
  colorp = _IGetColor ( color );
  ISetForeground ( gc, color );

  for ( row = src_y, y = dest_y; row < src_y + height; row++, y++ ) {
    for ( col = src_x, x = dest_x; col < src_x + width; col++, x++ ) {
      if ( i1->greyscale ) {
        ptr = i1->data + ( row * i1->width ) + col;
        colorp->red = colorp->green = colorp->blue = *(ptr);
        _ISetPoint ( i2, gcp, x, y );
      }
      else {
        ptr = i1->data + ( row * i1->width * 3 ) + ( col * 3 );
        /* check for transparent color */
        if ( i1->transparent == NULL 
          || i1->transparent->red != *(ptr)
          || i1->transparent->green != *(ptr + 1)
          || i1->transparent->blue != (*ptr + 2) ) {
          colorp->red = *(ptr);
          colorp->green = *(ptr + 1);
          colorp->blue = *(ptr + 2);
          _ISetPoint ( i2, gcp, x, y );
        }
      }
    }
  }

  IFreeColor ( color );

  gcp->foreground = save;

  return ( INoError );
}




/*
** This allows the user to scale up or down the source image onto
** the destination image.
*/
IError ICopyImageScaled ( source, dest, gc,
  src_x, src_y, src_width, src_height,
  dest_x, dest_y, dest_width, dest_height )
IImage source, dest;
IGC gc;
int src_x, src_y;
unsigned int src_width, src_height;
int dest_x, dest_y;
unsigned int dest_width, dest_height;
{
  IImageP *i1 = (IImageP *) source;
  IImageP *i2 = (IImageP *) dest;
  IGCP *gcp = (IGCP *) gc;
  int x, y, x2, y2;
  IColor color;
  IColorP *colorp;
  IColorP *save;
  unsigned char *ptr;
  double scalex, scaley;
  double tempx, tempy;

  if ( i1 ) {
    if ( i1->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( i2 ) {
    if ( i2->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( gcp ) {
    if ( gcp->magic != IMAGIC_GC )
      return ( IInvalidGC );
  }
  else
    return ( IInvalidGC );

  save = gcp->foreground;

  color = IAllocColor ( 0, 0, 0 );
  colorp = _IGetColor ( color );
  ISetForeground ( gc, color );

  /*
  ** When scaling down, we might want to add an algorithm for averaging
  ** a series of source pixels into the destination pixel.  For now,
  ** we just grab one color.
  */
  scalex = (double) dest_width / (double) src_width;
  scaley = (double) dest_height / (double) src_height;
  for ( y = dest_y; y < dest_y + dest_height; y++ ) {
    for ( x = dest_x; x < dest_x + dest_width; x++ ) {
      /* get location from source image for this location
      ** x2,y2 is location in source image.
      */
      tempx = (double) src_x + (double) ( x - dest_x ) / scalex;
      x2 = (int) tempx;
      tempy = (double) src_y + (double) ( y - dest_y ) / scaley;
      y2 = (int) tempy;
      if ( i1->greyscale ) {
        ptr = i1->data + ( y2 * i1->width ) + x2;
        colorp->red = colorp->green = colorp->blue = *(ptr);
        _ISetPoint ( i2, gcp, x, y );
      }
      else {
        ptr = i1->data + ( y2 * i1->width * 3 ) + ( x2 * 3 );
        /* check for transparent color */
        if ( i1->transparent == NULL 
          || i1->transparent->red != *(ptr)
          || i1->transparent->green != *(ptr + 1)
          || i1->transparent->blue != *(ptr + 2) ) {
          colorp->red = *(ptr);
          colorp->green = *(ptr + 1);
          colorp->blue = *(ptr + 2);
          _ISetPoint ( i2, gcp, x, y );
        }
      }
    }
  }

  IFreeColor ( color );

  gcp->foreground = save;

  return ( INoError );
}


