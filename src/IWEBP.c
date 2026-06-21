/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IWEBP.c
 *
 * Image library
 *
 * Description:
 *	Read and write WebP images using Google's libwebp. Decoding produces an
 *	RGB image, or RGBA when the file carries an alpha channel. Encoding takes
 *	RGB/RGBA directly (greyscale images are expanded to RGB) and uses lossy
 *	compression at a fixed high quality.
 *
 ****************************************************************************/

#ifdef HAVE_WEBPLIB

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <webp/decode.h>
#include <webp/encode.h>

#include "Ilib.h"
#include "IlibP.h"

#define IWEBP_QUALITY 90.0f /* lossy quality factor (0..100) */

/* Read an entire stream into a malloc'd buffer. Returns the buffer (caller
   frees) and sets *size_return, or NULL on allocation failure. */
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
      break; /* EOF or read error */
  }
  *size_return = len;
  return ( buf );
}

IError _IReadWEBP ( FILE *fp, IOptions options, IImageP **image_return )
{
  unsigned char *data;
  size_t size = 0;
  int width = 0, height = 0, channels;
  WebPBitstreamFeatures features;
  IImageP *image;
  (void) options;

  data = _IReadAll ( fp, &size );
  if ( !data )
    return ( IWEBPError );

  if ( size == 0 || !WebPGetInfo ( data, size, &width, &height ) ) {
    free ( data );
    return ( IFileInvalid );
  }
  if ( WebPGetFeatures ( data, size, &features ) != VP8_STATUS_OK ) {
    free ( data );
    return ( IFileInvalid );
  }

  channels = features.has_alpha ? 4 : 3;
  image = (IImageP *) ICreateImage ( width, height,
    channels == 4 ? IOPTION_ALPHA : IOPTION_NONE );
  if ( !image ) {
    free ( data );
    return ( IWEBPError );
  }

  if ( channels == 4 ) {
    if ( !WebPDecodeRGBAInto ( data, size, image->data,
           (size_t) width * height * 4, width * 4 ) ) {
      _IFreeImage ( (IImage) image );
      free ( data );
      return ( IWEBPError );
    }
  }
  else {
    if ( !WebPDecodeRGBInto ( data, size, image->data,
           (size_t) width * height * 3, width * 3 ) ) {
      _IFreeImage ( (IImage) image );
      free ( data );
      return ( IWEBPError );
    }
  }

  free ( data );
  *image_return = image;
  return ( INoError );
}

IError _IWriteWEBP ( FILE *fp, IImageP *image, IOptions options )
{
  uint8_t *out = NULL;
  size_t out_size = 0;
  int w = image->width, h = image->height;
  (void) options;

  if ( image->greyscale ) {
    /* libwebp has no greyscale mode; expand the single channel to RGB. */
    size_t n = (size_t) w * h, i;
    unsigned char *rgb = (unsigned char *) malloc ( n * 3 );
    if ( !rgb )
      return ( IWEBPError );
    for ( i = 0; i < n; i++ ) {
      unsigned char v = image->data[i];
      rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = v;
    }
    out_size = WebPEncodeRGB ( rgb, w, h, w * 3, IWEBP_QUALITY, &out );
    free ( rgb );
  }
  else if ( image->has_alpha ) {
    out_size = WebPEncodeRGBA ( image->data, w, h, w * 4, IWEBP_QUALITY, &out );
  }
  else {
    out_size = WebPEncodeRGB ( image->data, w, h, w * 3, IWEBP_QUALITY, &out );
  }

  if ( out_size == 0 || !out ) {
    if ( out )
      WebPFree ( out );
    return ( IWEBPError );
  }
  if ( fwrite ( out, 1, out_size, fp ) != out_size ) {
    WebPFree ( out );
    return ( IErrorWriting );
  }
  fflush ( fp );
  WebPFree ( out );
  return ( INoError );
}

#endif /* HAVE_WEBPLIB */
