/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ICompose.c
 *
 * Image library
 *
 * Description:
 *	Composition operations: trim (autocrop a uniform border), border (add a
 *	solid frame), and the multi-image layouts append (side-by-side or stacked)
 *	and montage (a grid / contact sheet). ITrim and IBorder modify their image
 *	in place; IAppend and IMontage build and return a new image.
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

/* Fill a freshly allocated pixel buffer with a solid colour. */
static void _IFillBuffer ( unsigned char *data, int w, int h, int bpp,
  int greyscale, IColorP *c )
{
  size_t npixels = (size_t) w * h, i;

  if ( greyscale ) {
    for ( i = 0; i < npixels; i++ )
      data[i] = c->red;
  }
  else {
    for ( i = 0; i < npixels; i++ ) {
      unsigned char *p = data + i * bpp;
      p[0] = c->red;
      p[1] = c->green;
      p[2] = c->blue;
      if ( bpp == 4 )
        p[3] = c->alpha;
    }
  }
}

/* Copy all of src onto dst at (dx, dy), channel-safe (handles grey/RGB/RGBA
   mixes) via the public pixel accessors. Out-of-range writes are dropped. */
static void _ICopyRegion ( IImage src, IImage dst, int dx, int dy )
{
  unsigned int w = IImageWidth ( src ), h = IImageHeight ( src ), x, y;

  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      unsigned int r, g, b, a;
      IGetPixelAlpha ( src, (int) x, (int) y, &r, &g, &b, &a );
      ISetPixelAlpha ( dst, dx + (int) x, dy + (int) y, r, g, b, a );
    }
  }
}

IError ITrim ( IImage image, unsigned int tolerance )
{
  IImageP *imagep = (IImageP *) image;
  int w, h, x, y;
  int minx, miny, maxx, maxy;
  unsigned int br, bg, bb, ba;
  int tol = (int) tolerance;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  w = imagep->width;
  h = imagep->height;
  /* Use the top-left corner as the border colour to strip (like -trim). */
  IGetPixelAlpha ( image, 0, 0, &br, &bg, &bb, &ba );

  minx = w;
  miny = h;
  maxx = -1;
  maxy = -1;
  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      unsigned int r, g, b, a;
      IGetPixelAlpha ( image, x, y, &r, &g, &b, &a );
      if ( abs ( (int) r - (int) br ) > tol ||
           abs ( (int) g - (int) bg ) > tol ||
           abs ( (int) b - (int) bb ) > tol ||
           abs ( (int) a - (int) ba ) > tol ) {
        if ( x < minx )
          minx = x;
        if ( x > maxx )
          maxx = x;
        if ( y < miny )
          miny = y;
        if ( y > maxy )
          maxy = y;
      }
    }
  }

  if ( maxx < 0 )
    return ( INoError ); /* entirely the border colour: nothing to trim */
  if ( minx == 0 && miny == 0 && maxx == w - 1 && maxy == h - 1 )
    return ( INoError ); /* no uniform border */

  return (
    ICrop ( image, minx, miny, (unsigned int) ( maxx - minx + 1 ),
      (unsigned int) ( maxy - miny + 1 ) ) );
}

IError IBorder ( IImage image, unsigned int width, IColor color )
{
  IImageP *imagep = (IImageP *) image;
  IColorP *c;
  int w, h, bpp, bw, nw, nh, y;
  unsigned char *nd;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( width == 0 )
    return ( INoError );
  c = _IGetColor ( (int) color );
  if ( !c )
    return ( IInvalidColor );

  w = imagep->width;
  h = imagep->height;
  bpp = imagep->channels;
  bw = (int) width;
  nw = w + 2 * bw;
  nh = h + 2 * bw;

  nd = (unsigned char *) malloc ( (size_t) nw * nh * bpp );
  if ( !nd )
    return ( IInvalidImage );

  _IFillBuffer ( nd, nw, nh, bpp, imagep->greyscale, c );
  /* Copy the original rows into the centre (same channel count -> memcpy). */
  for ( y = 0; y < h; y++ ) {
    unsigned char *dstrow = nd + ( (size_t) ( y + bw ) * nw + bw ) * bpp;
    unsigned char *srcrow = imagep->data + (size_t) y * w * bpp;
    memcpy ( dstrow, srcrow, (size_t) w * bpp );
  }

  free ( imagep->data );
  imagep->data = nd;
  imagep->width = nw;
  imagep->height = nh;
  return ( INoError );
}

/* Validate an array of image handles and report whether any has alpha. */
static int _IValidateImages ( IImage *images, int count, int *any_alpha )
{
  int i;
  *any_alpha = 0;
  if ( !images || count <= 0 )
    return ( 0 );
  for ( i = 0; i < count; i++ ) {
    IImageP *p = (IImageP *) images[i];
    if ( !p || p->magic != IMAGIC_IMAGE )
      return ( 0 );
    if ( p->has_alpha )
      *any_alpha = 1;
  }
  return ( 1 );
}

IImage IAppend ( IImage *images, int count, int horizontal, IColor background )
{
  IColorP *bg;
  IImageP *dst;
  int i, total = 0, other = 0, any_alpha = 0, off = 0;

  if ( !_IValidateImages ( images, count, &any_alpha ) )
    return ( NULL );
  bg = _IGetColor ( (int) background );
  if ( !bg )
    return ( NULL );

  for ( i = 0; i < count; i++ ) {
    IImageP *p = (IImageP *) images[i];
    if ( horizontal ) {
      total += p->width;
      if ( p->height > other )
        other = p->height;
    }
    else {
      total += p->height;
      if ( p->width > other )
        other = p->width;
    }
  }

  dst = (IImageP *) ICreateImage ( horizontal ? total : other,
    horizontal ? other : total, any_alpha ? IOPTION_ALPHA : IOPTION_NONE );
  if ( !dst )
    return ( NULL );
  _IFillBuffer ( dst->data, dst->width, dst->height, dst->channels,
    dst->greyscale, bg );

  for ( i = 0; i < count; i++ ) {
    IImageP *p = (IImageP *) images[i];
    if ( horizontal ) {
      _ICopyRegion ( images[i], (IImage) dst, off, 0 );
      off += p->width;
    }
    else {
      _ICopyRegion ( images[i], (IImage) dst, 0, off );
      off += p->height;
    }
  }
  return ( (IImage) dst );
}

IImage IMontage ( IImage *images, int count, int columns, int spacing,
  IColor background )
{
  IColorP *bg;
  IImageP *dst;
  int i, cellw = 0, cellh = 0, rows, nw, nh, any_alpha = 0;

  if ( !_IValidateImages ( images, count, &any_alpha ) )
    return ( NULL );
  if ( columns <= 0 || spacing < 0 )
    return ( NULL );
  bg = _IGetColor ( (int) background );
  if ( !bg )
    return ( NULL );

  /* Uniform cells sized to the largest input. */
  for ( i = 0; i < count; i++ ) {
    IImageP *p = (IImageP *) images[i];
    if ( p->width > cellw )
      cellw = p->width;
    if ( p->height > cellh )
      cellh = p->height;
  }
  rows = ( count + columns - 1 ) / columns;
  nw = columns * cellw + ( columns + 1 ) * spacing;
  nh = rows * cellh + ( rows + 1 ) * spacing;

  dst = (IImageP *) ICreateImage ( nw, nh,
    any_alpha ? IOPTION_ALPHA : IOPTION_NONE );
  if ( !dst )
    return ( NULL );
  _IFillBuffer ( dst->data, nw, nh, dst->channels, dst->greyscale, bg );

  for ( i = 0; i < count; i++ ) {
    IImageP *p = (IImageP *) images[i];
    int col = i % columns, row = i / columns;
    int cellx = spacing + col * ( cellw + spacing );
    int celly = spacing + row * ( cellh + spacing );
    /* Centre each image within its cell. */
    int dx = cellx + ( cellw - p->width ) / 2;
    int dy = celly + ( cellh - p->height ) / 2;
    _ICopyRegion ( images[i], (IImage) dst, dx, dy );
  }
  return ( (IImage) dst );
}
