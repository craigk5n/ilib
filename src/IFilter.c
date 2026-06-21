/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IFilter.c
 *
 * Image library
 *
 * Description:
 *	Whole-image point operations (ImageMagick-style filters). Each pixel's
 *	new value depends only on its old value, so most of these are driven by
 *	a single 256-entry lookup table built once and applied to every colour
 *	channel via _IApplyLUT(). Greyscale conversion is the exception: it mixes
 *	the three channels, so it has its own per-pixel loop.
 *
 *	Operations modify the image in place and leave the alpha channel
 *	untouched.
 *
 ****************************************************************************/

#include <math.h>
#include <stdio.h>

#include "Ilib.h"
#include "IlibP.h"

/*
 * Apply a 256-entry lookup table to every colour channel of the image.
 * For a greyscale image the single stored channel is mapped; for RGB/RGBA the
 * first three channels of each pixel are mapped and any alpha is left alone.
 */
static void _IApplyLUT ( IImageP *image, const unsigned char lut[256] )
{
  size_t npixels = (size_t) image->width * image->height;
  unsigned char *data = image->data;
  size_t i;

  if ( image->greyscale ) {
    for ( i = 0; i < npixels; i++ )
      data[i] = lut[data[i]];
  }
  else {
    unsigned int ch = image->channels;
    unsigned int color_ch = ( ch >= 3 ) ? 3 : ch; /* skip alpha at index 3 */
    for ( i = 0; i < npixels; i++ ) {
      unsigned char *p = data + i * ch;
      unsigned int c;
      for ( c = 0; c < color_ch; c++ )
        p[c] = lut[p[c]];
    }
  }
}

/* Common handle validation for the filters below. */
static IError _IValidImage ( IImageP *image )
{
  if ( !image )
    return ( IInvalidImage );
  if ( image->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  return ( INoError );
}

IError IGreyscale ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  size_t npixels, i;
  unsigned int ch, color_ch;
  unsigned char *data;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  /* Already a single-channel greyscale image: nothing to do. */
  if ( imagep->greyscale )
    return ( INoError );

  npixels = (size_t) imagep->width * imagep->height;
  data = imagep->data;
  ch = imagep->channels;
  color_ch = ( ch >= 3 ) ? 3 : ch;

  if ( color_ch < 3 )
    return ( INoError ); /* not enough channels to desaturate */

  for ( i = 0; i < npixels; i++ ) {
    unsigned char *p = data + i * ch;
    /* Rec.601 luma with integer weights summing to 256. */
    unsigned int luma = ( p[0] * 77u + p[1] * 150u + p[2] * 29u ) >> 8;
    p[0] = p[1] = p[2] = (unsigned char) luma;
  }
  return ( INoError );
}

IError INegate ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char lut[256];
  int i;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  for ( i = 0; i < 256; i++ )
    lut[i] = (unsigned char) ( 255 - i );
  _IApplyLUT ( imagep, lut );
  return ( INoError );
}

/* Clamp an int to the 0..255 byte range. */
static unsigned char _IClampByte ( int v )
{
  if ( v < 0 )
    return ( 0 );
  if ( v > 255 )
    return ( 255 );
  return ( (unsigned char) v );
}

/* Clamp an int to a closed range. */
static int _IClampInt ( int v, int lo, int hi )
{
  if ( v < lo )
    return ( lo );
  if ( v > hi )
    return ( hi );
  return ( v );
}

IError IBrightnessContrast ( IImage image, int brightness, int contrast )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char lut[256];
  int offset, c255, i;
  double factor;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  /* Accept -100..100 for both controls; clamp anything outside. */
  brightness = _IClampInt ( brightness, -100, 100 );
  contrast = _IClampInt ( contrast, -100, 100 );

  /* Brightness: additive shift in -255..255. */
  offset = brightness * 255 / 100;

  /* Contrast: the standard factor formula, contrast scaled to -255..255. */
  c255 = contrast * 255 / 100;
  if ( c255 >= 259 )
    c255 = 258;
  factor = ( 259.0 * ( c255 + 255 ) ) / ( 255.0 * ( 259 - c255 ) );

  for ( i = 0; i < 256; i++ ) {
    int v = (int) ( factor * ( i - 128 ) + 128.0 + 0.5 ) + offset;
    lut[i] = _IClampByte ( v );
  }
  _IApplyLUT ( imagep, lut );
  return ( INoError );
}

IError IGamma ( IImage image, double gamma )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char lut[256];
  int i;
  double inv;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( !( gamma > 0.0 ) ) /* also rejects NaN */
    return ( IInvalidArgument );

  inv = 1.0 / gamma;
  for ( i = 0; i < 256; i++ ) {
    double out = 255.0 * pow ( i / 255.0, inv );
    lut[i] = _IClampByte ( (int) ( out + 0.5 ) );
  }
  _IApplyLUT ( imagep, lut );
  return ( INoError );
}

IError IThreshold ( IImage image, unsigned int threshold )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char lut[256];
  int i;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( threshold > 255 )
    return ( IInvalidArgument );

  for ( i = 0; i < 256; i++ )
    lut[i] = ( (unsigned int) i >= threshold ) ? 255 : 0;
  _IApplyLUT ( imagep, lut );
  return ( INoError );
}

/* Find the min and max colour-channel value used anywhere in the image. */
static void _IMinMax ( IImageP *image, int *min_return, int *max_return )
{
  size_t npixels = (size_t) image->width * image->height;
  unsigned char *data = image->data;
  int lo = 255, hi = 0;
  size_t i;

  if ( image->greyscale ) {
    for ( i = 0; i < npixels; i++ ) {
      int v = data[i];
      if ( v < lo )
        lo = v;
      if ( v > hi )
        hi = v;
    }
  }
  else {
    unsigned int ch = image->channels;
    unsigned int color_ch = ( ch >= 3 ) ? 3 : ch;
    for ( i = 0; i < npixels; i++ ) {
      unsigned char *p = data + i * ch;
      unsigned int c;
      for ( c = 0; c < color_ch; c++ ) {
        int v = p[c];
        if ( v < lo )
          lo = v;
        if ( v > hi )
          hi = v;
      }
    }
  }
  *min_return = lo;
  *max_return = hi;
}

IError INormalize ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  unsigned char lut[256];
  int i, lo, hi, range;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  _IMinMax ( imagep, &lo, &hi );
  range = hi - lo;
  if ( range <= 0 )
    return ( INoError ); /* flat image: nothing to stretch */

  for ( i = 0; i < 256; i++ ) {
    int v = ( i - lo ) * 255 / range;
    lut[i] = _IClampByte ( v );
  }
  _IApplyLUT ( imagep, lut );
  return ( INoError );
}

IError ISepia ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  size_t npixels, i;
  unsigned int ch, color_ch;
  unsigned char *data;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  /* Sepia mixes the three channels, so it needs a colour image. */
  if ( imagep->greyscale )
    return ( INoError );
  ch = imagep->channels;
  color_ch = ( ch >= 3 ) ? 3 : ch;
  if ( color_ch < 3 )
    return ( INoError );

  npixels = (size_t) imagep->width * imagep->height;
  data = imagep->data;
  for ( i = 0; i < npixels; i++ ) {
    unsigned char *p = data + i * ch;
    int r = p[0], g = p[1], b = p[2];
    /* Standard sepia matrix (weights x1000). */
    int tr = ( r * 393 + g * 769 + b * 189 ) / 1000;
    int tg = ( r * 349 + g * 686 + b * 168 ) / 1000;
    int tb = ( r * 272 + g * 534 + b * 131 ) / 1000;
    p[0] = _IClampByte ( tr );
    p[1] = _IClampByte ( tg );
    p[2] = _IClampByte ( tb );
  }
  return ( INoError );
}

IError IOpacity ( IImage image, double factor )
{
  IImageP *imagep = (IImageP *) image;
  size_t npixels, i;
  unsigned char *data;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( !( factor >= 0.0 ) ) /* also rejects NaN */
    return ( IInvalidArgument );

  /* Only RGBA images have an alpha channel to scale. */
  if ( !imagep->has_alpha || imagep->channels != 4 )
    return ( INoError );

  npixels = (size_t) imagep->width * imagep->height;
  data = imagep->data;
  for ( i = 0; i < npixels; i++ ) {
    unsigned char *p = data + i * 4;
    int a = (int) ( p[3] * factor + 0.5 );
    p[3] = _IClampByte ( a );
  }
  return ( INoError );
}
