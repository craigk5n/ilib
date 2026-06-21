/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IAVIF.c
 *
 * Image library
 *
 * Description:
 *	Read and write AVIF images using libavif. Decoding produces an RGB image,
 *	or RGBA when the file carries an alpha channel; high-bit-depth AVIFs are
 *	down-converted to 8-bit. Encoding takes RGB/RGBA directly (greyscale
 *	images are expanded to RGB) and uses lossy compression at a fixed
 *	quality.
 *
 ****************************************************************************/

#ifdef HAVE_AVIFLIB

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avif/avif.h>

#include "Ilib.h"
#include "IlibP.h"

#define IAVIF_QUALITY 60 /* 0 (worst) .. 100 (best/lossless) */
#define IAVIF_SPEED 6    /* 0 (slowest/best) .. 10 (fastest) */

/* Read an entire stream into a malloc'd buffer (caller frees), *size set. */
static unsigned char *_IReadAll ( FILE *fp, size_t *size_return )
{
  size_t cap = 65536, len = 0;
  unsigned char *buf = (unsigned char *) malloc ( cap );

  if ( !buf )
    return ( NULL );

  for ( ;; ) {
    size_t n;
    if ( len == cap ) {
      unsigned char *nb = (unsigned char *) realloc ( buf, cap * 2 );
      if ( !nb ) {
        free ( buf );
        return ( NULL );
      }
      buf = nb;
      cap *= 2;
    }
    n = fread ( buf + len, 1, cap - len, fp );
    len += n;
    if ( n == 0 )
      break;
  }
  *size_return = len;
  return ( buf );
}

IError _IReadAVIF ( FILE *fp, IOptions options, IImageP **image_return )
{
  unsigned char *data;
  size_t size = 0;
  avifDecoder *decoder;
  avifRGBImage rgb;
  avifResult r;
  IImageP *image;
  int channels;
  (void) options;

  data = _IReadAll ( fp, &size );
  if ( !data )
    return ( IAVIFError );

  decoder = avifDecoderCreate ();
  if ( !decoder ) {
    free ( data );
    return ( IAVIFError );
  }

  if ( avifDecoderSetIOMemory ( decoder, data, size ) != AVIF_RESULT_OK ||
       avifDecoderParse ( decoder ) != AVIF_RESULT_OK ||
       avifDecoderNextImage ( decoder ) != AVIF_RESULT_OK ) {
    avifDecoderDestroy ( decoder );
    free ( data );
    return ( IFileInvalid );
  }

  channels = ( decoder->image->alphaPlane != NULL ) ? 4 : 3;
  image = (IImageP *) ICreateImage ( decoder->image->width,
    decoder->image->height, channels == 4 ? IOPTION_ALPHA : IOPTION_NONE );
  if ( !image ) {
    avifDecoderDestroy ( decoder );
    free ( data );
    return ( IAVIFError );
  }

  /* Convert YUV -> RGB(A) straight into our 8-bit buffer (no extra copy). */
  memset ( &rgb, 0, sizeof ( rgb ) );
  avifRGBImageSetDefaults ( &rgb, decoder->image );
  rgb.format = ( channels == 4 ) ? AVIF_RGB_FORMAT_RGBA : AVIF_RGB_FORMAT_RGB;
  rgb.depth = 8;
  rgb.pixels = image->data;
  rgb.rowBytes = (uint32_t) ( image->width * channels );

  r = avifImageYUVToRGB ( decoder->image, &rgb );
  avifDecoderDestroy ( decoder );
  free ( data );
  if ( r != AVIF_RESULT_OK ) {
    _IFreeImage ( (IImage) image );
    return ( IAVIFError );
  }

  *image_return = image;
  return ( INoError );
}

IError _IWriteAVIF ( FILE *fp, IImageP *image, IOptions options )
{
  int w = image->width, h = image->height;
  int channels = image->has_alpha ? 4 : 3;
  unsigned char *rgbbuf = image->data;
  unsigned char *tmp = NULL;
  avifImage *aimage;
  avifRGBImage rgb;
  avifEncoder *encoder;
  avifResult r;
  avifRWData out = AVIF_DATA_EMPTY;
  IError err = INoError;
  (void) options;

  if ( image->greyscale ) {
    /* libavif encodes from RGB(A); expand the single channel to RGB. */
    size_t n = (size_t) w * h, i;
    tmp = (unsigned char *) malloc ( n * 3 );
    if ( !tmp )
      return ( IAVIFError );
    for ( i = 0; i < n; i++ ) {
      unsigned char v = image->data[i];
      tmp[i * 3] = tmp[i * 3 + 1] = tmp[i * 3 + 2] = v;
    }
    rgbbuf = tmp;
    channels = 3;
  }

  aimage = avifImageCreate ( w, h, 8, AVIF_PIXEL_FORMAT_YUV444 );
  if ( !aimage ) {
    free ( tmp );
    return ( IAVIFError );
  }

  memset ( &rgb, 0, sizeof ( rgb ) );
  avifRGBImageSetDefaults ( &rgb, aimage );
  rgb.format = ( channels == 4 ) ? AVIF_RGB_FORMAT_RGBA : AVIF_RGB_FORMAT_RGB;
  rgb.depth = 8;
  rgb.pixels = rgbbuf;
  rgb.rowBytes = (uint32_t) ( w * channels );

  if ( avifImageRGBToYUV ( aimage, &rgb ) != AVIF_RESULT_OK ) {
    avifImageDestroy ( aimage );
    free ( tmp );
    return ( IAVIFError );
  }

  encoder = avifEncoderCreate ();
  if ( !encoder ) {
    avifImageDestroy ( aimage );
    free ( tmp );
    return ( IAVIFError );
  }
  encoder->speed = IAVIF_SPEED;
#if defined( AVIF_VERSION ) && AVIF_VERSION >= 110000
  /* The simple quality knob (0..100) was added in libavif 0.11.0. */
  encoder->quality = IAVIF_QUALITY;
  encoder->qualityAlpha = IAVIF_QUALITY;
#else
  {
    /* Older libavif: map quality to a quantizer (0 best .. 63 worst). */
    int q = ( 100 - IAVIF_QUALITY ) * AVIF_QUANTIZER_WORST_QUALITY / 100;
    encoder->minQuantizer = q;
    encoder->maxQuantizer = q;
    encoder->minQuantizerAlpha = q;
    encoder->maxQuantizerAlpha = q;
  }
#endif

  r = avifEncoderWrite ( encoder, aimage, &out );
  if ( r == AVIF_RESULT_OK ) {
    if ( fwrite ( out.data, 1, out.size, fp ) != out.size )
      err = IErrorWriting;
    else
      fflush ( fp );
  }
  else {
    err = IAVIFError;
  }

  avifRWDataFree ( &out );
  avifEncoderDestroy ( encoder );
  avifImageDestroy ( aimage );
  free ( tmp );
  return ( err );
}

#endif /* HAVE_AVIFLIB */
