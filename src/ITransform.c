/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ITransform.c
 *
 * Image library
 *
 * Description:
 *	Geometric whole-image transforms (ImageMagick-style): flip, flop,
 *	rotate by a multiple of 90 degrees, and crop. These only move pixels
 *	around, so they are channel-agnostic: the per-pixel stride is the image's
 *	channel count (1 = greyscale, 3 = RGB, 4 = RGBA).
 *
 *	Operations modify the image in place. The ones that change the image
 *	dimensions (rotate by 90/270 and crop) allocate a new pixel buffer, fill
 *	it, then swap it in; on allocation failure the image is left unchanged
 *	and IInvalidImage is returned (matching IDuplicateImage's convention).
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

static IError _IValidImage ( IImageP *image )
{
  if ( !image )
    return ( IInvalidImage );
  if ( image->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  return ( INoError );
}

IError IFlip ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  int y, h, half;
  size_t rowbytes;
  unsigned char *tmp, *data;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  h = imagep->height;
  data = imagep->data;
  rowbytes = (size_t) imagep->width * imagep->channels;
  half = h / 2;

  tmp = (unsigned char *) malloc ( rowbytes );
  if ( !tmp )
    return ( IInvalidImage );

  for ( y = 0; y < half; y++ ) {
    unsigned char *top = data + (size_t) y * rowbytes;
    unsigned char *bot = data + (size_t) ( h - 1 - y ) * rowbytes;
    memcpy ( tmp, top, rowbytes );
    memcpy ( top, bot, rowbytes );
    memcpy ( bot, tmp, rowbytes );
  }
  free ( tmp );
  return ( INoError );
}

IError IFlop ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  int x, y, w, h, half;
  size_t bpp;
  unsigned char *tmp, *data;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  w = imagep->width;
  h = imagep->height;
  data = imagep->data;
  bpp = imagep->channels;
  half = w / 2;

  tmp = (unsigned char *) malloc ( bpp );
  if ( !tmp )
    return ( IInvalidImage );

  for ( y = 0; y < h; y++ ) {
    unsigned char *row = data + (size_t) y * w * bpp;
    for ( x = 0; x < half; x++ ) {
      unsigned char *l = row + (size_t) x * bpp;
      unsigned char *r = row + (size_t) ( w - 1 - x ) * bpp;
      memcpy ( tmp, l, bpp );
      memcpy ( l, r, bpp );
      memcpy ( r, tmp, bpp );
    }
  }
  free ( tmp );
  return ( INoError );
}

/* Rotate 180 degrees: reverse the whole pixel sequence in place. */
static IError _IRotate180 ( IImageP *image )
{
  size_t bpp = image->channels;
  size_t n = (size_t) image->width * image->height;
  size_t i, half = n / 2;
  unsigned char *data = image->data;
  unsigned char *tmp = (unsigned char *) malloc ( bpp );

  if ( !tmp )
    return ( IInvalidImage );

  for ( i = 0; i < half; i++ ) {
    unsigned char *a = data + i * bpp;
    unsigned char *b = data + ( n - 1 - i ) * bpp;
    memcpy ( tmp, a, bpp );
    memcpy ( a, b, bpp );
    memcpy ( b, tmp, bpp );
  }
  free ( tmp );
  return ( INoError );
}

/* Rotate a quarter turn into a fresh buffer; cw != 0 means clockwise. */
static IError _IRotateQuarter ( IImageP *image, int cw )
{
  int w = image->width, h = image->height;
  size_t bpp = image->channels;
  int nw = h, nh = w; /* dimensions swap */
  int x, y;
  unsigned char *nd = (unsigned char *) malloc ( (size_t) w * h * bpp );

  if ( !nd )
    return ( IInvalidImage );

  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      const unsigned char *src = image->data + ( (size_t) y * w + x ) * bpp;
      int nx, ny;
      unsigned char *dst;
      if ( cw ) { /* clockwise: (x,y) -> (h-1-y, x) */
        nx = h - 1 - y;
        ny = x;
      }
      else { /* counter-clockwise: (x,y) -> (y, w-1-x) */
        nx = y;
        ny = w - 1 - x;
      }
      dst = nd + ( (size_t) ny * nw + nx ) * bpp;
      memcpy ( dst, src, bpp );
    }
  }
  free ( image->data );
  image->data = nd;
  image->width = nw;
  image->height = nh;
  return ( INoError );
}

IError IRotate ( IImage image, int degrees )
{
  IImageP *imagep = (IImageP *) image;
  int d;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  d = ( ( degrees % 360 ) + 360 ) % 360;
  switch ( d ) {
  case 0:
    return ( INoError );
  case 90:
    return ( _IRotateQuarter ( imagep, 1 ) );
  case 180:
    return ( _IRotate180 ( imagep ) );
  case 270:
    return ( _IRotateQuarter ( imagep, 0 ) );
  default:
    return ( IInvalidArgument );
  }
}

IError ICrop ( IImage image, int x, int y, unsigned int width,
  unsigned int height )
{
  IImageP *imagep = (IImageP *) image;
  size_t bpp, rowbytes;
  unsigned int ny;
  unsigned char *nd;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  if ( width == 0 || height == 0 )
    return ( IInvalidArgument );
  if ( x < 0 || y < 0 )
    return ( IInvalidArgument );
  if ( (size_t) x + width > (size_t) imagep->width )
    return ( IInvalidArgument );
  if ( (size_t) y + height > (size_t) imagep->height )
    return ( IInvalidArgument );

  bpp = imagep->channels;
  rowbytes = (size_t) width * bpp;

  nd = (unsigned char *) malloc ( (size_t) width * height * bpp );
  if ( !nd )
    return ( IInvalidImage );

  for ( ny = 0; ny < height; ny++ ) {
    const unsigned char *src =
      imagep->data + ( (size_t) ( y + ny ) * imagep->width + x ) * bpp;
    unsigned char *dst = nd + (size_t) ny * rowbytes;
    memcpy ( dst, src, rowbytes );
  }
  free ( imagep->data );
  imagep->data = nd;
  imagep->width = (int) width;
  imagep->height = (int) height;
  return ( INoError );
}
