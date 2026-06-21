/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IConvolve.c
 *
 * Image library
 *
 * Description:
 *	Spatial convolution and the area filters built on it (blur, sharpen,
 *	edge detect, emboss) -- the third batch of ImageMagick-style transforms.
 *
 *	A single engine (_IConvolveKernel) applies an N x N kernel to every
 *	colour channel of every pixel, reading from a saved copy of the image so
 *	neighbouring results do not feed back into each other. Out-of-edge
 *	samples use clamp-to-edge addressing (no dark border). The alpha channel
 *	of an RGBA image is left untouched.
 *
 *	out = (sum(kernel[i] * neighbour[i]) / divisor) + bias, clamped to 0..255.
 *	A divisor of 0 means "use the sum of the kernel weights" (or 1 if that
 *	sum is 0, e.g. an edge kernel).
 *
 ****************************************************************************/

#include <math.h>
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

/* Clamp v to [0, hi-1] for clamp-to-edge sampling. */
static int _IClampCoord ( int v, int hi )
{
  if ( v < 0 )
    return ( 0 );
  if ( v >= hi )
    return ( hi - 1 );
  return ( v );
}

/* Apply an odd-sized kernel to the colour channels of image, in place. */
static IError _IConvolveKernel ( IImageP *image, const double *kernel, int size,
  double divisor, double bias )
{
  int w = image->width, h = image->height;
  int bpp = (int) image->channels;
  int color_ch = ( bpp >= 3 ) ? 3 : bpp; /* leave any alpha untouched */
  int half = size / 2;
  int x, y, c, ky, kx;
  unsigned char *src;

  if ( divisor == 0.0 ) {
    int i, n = size * size;
    double sum = 0.0;
    for ( i = 0; i < n; i++ )
      sum += kernel[i];
    divisor = ( sum == 0.0 ) ? 1.0 : sum;
  }

  /* Read from a private copy so results never feed back during the pass. */
  src = (unsigned char *) malloc ( (size_t) w * h * bpp );
  if ( !src )
    return ( IInvalidImage );
  memcpy ( src, image->data, (size_t) w * h * bpp );

  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      unsigned char *out = image->data + ( (size_t) y * w + x ) * bpp;
      for ( c = 0; c < color_ch; c++ ) {
        double acc = 0.0;
        int v;
        for ( ky = 0; ky < size; ky++ ) {
          int sy = _IClampCoord ( y + ky - half, h );
          for ( kx = 0; kx < size; kx++ ) {
            int sx = _IClampCoord ( x + kx - half, w );
            const unsigned char *p = src + ( (size_t) sy * w + sx ) * bpp;
            acc += kernel[ky * size + kx] * p[c];
          }
        }
        v = (int) ( acc / divisor + bias + 0.5 );
        if ( v < 0 )
          v = 0;
        else if ( v > 255 )
          v = 255;
        out[c] = (unsigned char) v;
      }
    }
  }
  free ( src );
  return ( INoError );
}

IError IConvolve ( IImage image, const double *kernel, unsigned int size,
  double divisor, double bias )
{
  IImageP *imagep = (IImageP *) image;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( !kernel || size == 0 || ( size % 2 ) == 0 )
    return ( IInvalidArgument ); /* kernel must be a non-empty odd square */

  return ( _IConvolveKernel ( imagep, kernel, (int) size, divisor, bias ) );
}

IError IBlur ( IImage image, unsigned int radius )
{
  IImageP *imagep = (IImageP *) image;
  int size, n, i;
  double *kernel;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( radius == 0 )
    return ( INoError ); /* nothing to do */

  size = (int) ( 2 * radius + 1 );
  n = size * size;
  kernel = (double *) malloc ( (size_t) n * sizeof ( double ) );
  if ( !kernel )
    return ( IInvalidImage );
  for ( i = 0; i < n; i++ )
    kernel[i] = 1.0; /* box blur; divisor 0 -> averaged by the engine */

  err = _IConvolveKernel ( imagep, kernel, size, 0.0, 0.0 );
  free ( kernel );
  return ( err );
}

IError IGaussianBlur ( IImage image, double sigma )
{
  IImageP *imagep = (IImageP *) image;
  int radius, size, i, w, h, bpp, color_ch, x, y, c, k;
  double *kernel, sum, twoSigmaSq;
  unsigned char *tmp, *data;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( !( sigma > 0.0 ) ) /* also rejects NaN */
    return ( IInvalidArgument );

  radius = (int) ceil ( 2.0 * sigma );
  if ( radius < 1 )
    radius = 1;
  size = 2 * radius + 1;
  twoSigmaSq = 2.0 * sigma * sigma;

  /* A 2-D Gaussian is separable, so use one normalized 1-D kernel in two
     passes (horizontal then vertical): O(w*h*size) instead of O(w*h*size^2). */
  kernel = (double *) malloc ( (size_t) size * sizeof ( double ) );
  if ( !kernel )
    return ( IInvalidImage );
  sum = 0.0;
  for ( i = 0; i < size; i++ ) {
    int d = i - radius;
    kernel[i] = exp ( -(double) ( d * d ) / twoSigmaSq );
    sum += kernel[i];
  }
  for ( i = 0; i < size; i++ )
    kernel[i] /= sum;

  w = imagep->width;
  h = imagep->height;
  bpp = (int) imagep->channels;
  color_ch = ( bpp >= 3 ) ? 3 : bpp;
  data = imagep->data;

  tmp = (unsigned char *) malloc ( (size_t) w * h * bpp );
  if ( !tmp ) {
    free ( kernel );
    return ( IInvalidImage );
  }

  /* Horizontal pass: data -> tmp (alpha copied through). */
  for ( y = 0; y < h; y++ ) {
    const unsigned char *rowbase = data + (size_t) y * w * bpp;
    for ( x = 0; x < w; x++ ) {
      unsigned char *out = tmp + ( (size_t) y * w + x ) * bpp;
      for ( c = 0; c < color_ch; c++ ) {
        double acc = 0.0;
        int v;
        for ( k = 0; k < size; k++ ) {
          int sx = x + k - radius;
          if ( sx < 0 )
            sx = 0;
          else if ( sx > w - 1 )
            sx = w - 1;
          acc += kernel[k] * rowbase[(size_t) sx * bpp + c];
        }
        v = (int) ( acc + 0.5 );
        out[c] = (unsigned char) ( v < 0 ? 0 : ( v > 255 ? 255 : v ) );
      }
      if ( bpp == 4 )
        out[3] = rowbase[(size_t) x * bpp + 3];
    }
  }

  /* Vertical pass: tmp -> data (color channels only; alpha already in data). */
  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      unsigned char *out = data + ( (size_t) y * w + x ) * bpp;
      for ( c = 0; c < color_ch; c++ ) {
        double acc = 0.0;
        int v;
        for ( k = 0; k < size; k++ ) {
          int sy = y + k - radius;
          if ( sy < 0 )
            sy = 0;
          else if ( sy > h - 1 )
            sy = h - 1;
          acc += kernel[k] * tmp[( (size_t) sy * w + x ) * bpp + c];
        }
        v = (int) ( acc + 0.5 );
        out[c] = (unsigned char) ( v < 0 ? 0 : ( v > 255 ? 255 : v ) );
      }
    }
  }

  free ( tmp );
  free ( kernel );
  return ( INoError );
}

IError ISharpen ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  /* clang-format off */
  static const double kernel[9] = {
     0.0, -1.0,  0.0,
    -1.0,  5.0, -1.0,
     0.0, -1.0,  0.0
  };
  /* clang-format on */
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  return ( _IConvolveKernel ( imagep, kernel, 3, 1.0, 0.0 ) );
}

IError IEdgeDetect ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  /* clang-format off */
  static const double kernel[9] = {
    -1.0, -1.0, -1.0,
    -1.0,  8.0, -1.0,
    -1.0, -1.0, -1.0
  };
  /* clang-format on */
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  return ( _IConvolveKernel ( imagep, kernel, 3, 1.0, 0.0 ) );
}

IError IEmboss ( IImage image )
{
  IImageP *imagep = (IImageP *) image;
  /* clang-format off */
  static const double kernel[9] = {
    -2.0, -1.0,  0.0,
    -1.0,  0.0,  1.0,
     0.0,  1.0,  2.0
  };
  /* clang-format on */
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  /* Kernel sums to 0 and bias 128, so flat areas become mid-grey. */
  return ( _IConvolveKernel ( imagep, kernel, 3, 1.0, 128.0 ) );
}
