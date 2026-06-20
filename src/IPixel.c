/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IPixel.c
 *
 * Image library
 *
 * Description:
 *	Public per-pixel read/write accessors. These work directly in RGB
 *	(0-255 per channel) and are independent of any graphics context.
 *
 ****************************************************************************/

#include <stdio.h>

#include "Ilib.h"
#include "IlibP.h"

IError ISetPixel ( IImage image, int x, int y, unsigned int red,
  unsigned int green, unsigned int blue )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char *ptr;

  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( red > 255 || green > 255 || blue > 255 )
    return ( IInvalidArgument );
  if ( x < 0 || x >= imagep->width || y < 0 || y >= imagep->height )
    return ( IInvalidArgument );

  if ( imagep->greyscale ) {
    /* A greyscale image stores one value per pixel; use the red channel. */
    ptr = imagep->data + ( y * imagep->width ) + x;
    *ptr = (unsigned char) red;
  }
  else {
    ptr = imagep->data + ( ( y * imagep->width ) + x ) * 3;
    ptr[0] = (unsigned char) red;
    ptr[1] = (unsigned char) green;
    ptr[2] = (unsigned char) blue;
  }
  return ( INoError );
}

IError IGetPixel ( IImage image, int x, int y, unsigned int *red_return,
  unsigned int *green_return, unsigned int *blue_return )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char *ptr;
  unsigned int r, g, b;

  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( x < 0 || x >= imagep->width || y < 0 || y >= imagep->height )
    return ( IInvalidArgument );

  if ( imagep->greyscale ) {
    ptr = imagep->data + ( y * imagep->width ) + x;
    r = g = b = *ptr;
  }
  else {
    ptr = imagep->data + ( ( y * imagep->width ) + x ) * 3;
    r = ptr[0];
    g = ptr[1];
    b = ptr[2];
  }

  /* Any of the return pointers may be NULL to fetch only some channels. */
  if ( red_return )
    *red_return = r;
  if ( green_return )
    *green_return = g;
  if ( blue_return )
    *blue_return = b;
  return ( INoError );
}
