/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IPixel.c
 *
 * Image library
 *
 * Description:
 *	Public per-pixel read/write accessors and the source-over compositing
 *	primitive used by alpha-aware and anti-aliased drawing. Pixel values
 *	are RGB(A), 0-255 per channel, independent of any graphics context.
 *
 ****************************************************************************/

#include <stdio.h>

#include "Ilib.h"
#include "IlibP.h"

/* Offset of pixel (x,y) in image->data for a color (3- or 4-channel) image. */
#define IPIXEL_OFFSET( img, x, y ) \
  ( ( (size_t) ( y ) * ( img )->width + ( x ) ) * ( img )->channels )

IError ISetPixelAlpha ( IImage image, int x, int y, unsigned int red,
  unsigned int green, unsigned int blue, unsigned int alpha )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char *ptr;

  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( red > 255 || green > 255 || blue > 255 || alpha > 255 )
    return ( IInvalidArgument );
  if ( x < 0 || x >= imagep->width || y < 0 || y >= imagep->height )
    return ( IInvalidArgument );

  if ( imagep->greyscale ) {
    /* A greyscale image stores one value per pixel; use the red channel. */
    imagep->data[(size_t) y * imagep->width + x] = (unsigned char) red;
  }
  else {
    ptr = imagep->data + IPIXEL_OFFSET ( imagep, x, y );
    ptr[0] = (unsigned char) red;
    ptr[1] = (unsigned char) green;
    ptr[2] = (unsigned char) blue;
    if ( imagep->channels == 4 )
      ptr[3] = (unsigned char) alpha;
  }
  return ( INoError );
}

IError ISetPixel ( IImage image, int x, int y, unsigned int red,
  unsigned int green, unsigned int blue )
{
  /* Opaque by definition; alpha is stored as 255 on an RGBA image. */
  return ( ISetPixelAlpha ( image, x, y, red, green, blue, 255 ) );
}

IError IGetPixelAlpha ( IImage image, int x, int y, unsigned int *red_return,
  unsigned int *green_return, unsigned int *blue_return,
  unsigned int *alpha_return )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char *ptr;
  unsigned int r, g, b, a;

  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( x < 0 || x >= imagep->width || y < 0 || y >= imagep->height )
    return ( IInvalidArgument );

  if ( imagep->greyscale ) {
    r = g = b = imagep->data[(size_t) y * imagep->width + x];
    a = 255;
  }
  else {
    ptr = imagep->data + IPIXEL_OFFSET ( imagep, x, y );
    r = ptr[0];
    g = ptr[1];
    b = ptr[2];
    a = ( imagep->channels == 4 ) ? ptr[3] : 255;
  }

  /* Any of the return pointers may be NULL to fetch only some channels. */
  if ( red_return )
    *red_return = r;
  if ( green_return )
    *green_return = g;
  if ( blue_return )
    *blue_return = b;
  if ( alpha_return )
    *alpha_return = a;
  return ( INoError );
}

IError IGetPixel ( IImage image, int x, int y, unsigned int *red_return,
  unsigned int *green_return, unsigned int *blue_return )
{
  return (
    IGetPixelAlpha ( image, x, y, red_return, green_return, blue_return, NULL ) );
}

/* Round a/b for non-negative integers. */
#define IDIV255( n ) ( ( ( n ) + 127 ) / 255 )

void _IBlendPoint ( IImageP *image, IGCP *gc, int x, int y,
  unsigned int cover )
{
  IColorP *fg = gc->foreground;
  unsigned int sa, inv, sr, sg, sb;
  unsigned char *ptr;

  if ( x < 0 || x >= image->width || y < 0 || y >= image->height )
    return;
  if ( !fg )
    return;

  /* Effective source alpha = color alpha scaled by edge coverage. */
  sa = IDIV255 ( fg->alpha * cover );
  if ( sa == 0 )
    return;
  inv = 255 - sa;
  sr = fg->red;
  sg = fg->green;
  sb = fg->blue;

  if ( image->greyscale ) {
    /* Use the red channel as the greyscale source value. */
    unsigned char *p = image->data + (size_t) y * image->width + x;
    *p = (unsigned char) IDIV255 ( sr * sa + *p * inv );
    return;
  }

  ptr = image->data + IPIXEL_OFFSET ( image, x, y );

  if ( image->channels == 4 ) {
    /* Straight-alpha source-over onto an RGBA destination. */
    unsigned int da = ptr[3];
    unsigned int dcontrib = IDIV255 ( da * inv ); /* dst alpha after (1-sa) */
    unsigned int oa = sa + dcontrib;
    if ( oa == 0 ) {
      ptr[0] = ptr[1] = ptr[2] = ptr[3] = 0;
      return;
    }
    ptr[0] = (unsigned char) ( ( sr * sa + ptr[0] * dcontrib + oa / 2 ) / oa );
    ptr[1] = (unsigned char) ( ( sg * sa + ptr[1] * dcontrib + oa / 2 ) / oa );
    ptr[2] = (unsigned char) ( ( sb * sa + ptr[2] * dcontrib + oa / 2 ) / oa );
    ptr[3] = (unsigned char) oa;
  }
  else {
    /* Composite over an opaque RGB destination (stays opaque). */
    ptr[0] = (unsigned char) IDIV255 ( sr * sa + ptr[0] * inv );
    ptr[1] = (unsigned char) IDIV255 ( sg * sa + ptr[1] * inv );
    ptr[2] = (unsigned char) IDIV255 ( sb * sa + ptr[2] * inv );
  }
}
