/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ITIFF.c
 *
 * Image library
 *
 * Description:
 *	TIFF reading and writing via libtiff. Reading uses the high-level
 *	TIFFReadRGBAImageOriented() helper (so any TIFF flavour decodes) and keeps
 *	the RGB channels; writing emits an 8-bit, LZW-compressed, contiguous strip
 *	image (MINISBLACK for greyscale, RGB otherwise). TIFF is treated as an
 *	opaque (RGB) format here -- an RGBA image is flattened before writing, and
 *	the helper's premultiplied alpha is not round-tripped.
 *
 *	libtiff wants a file descriptor it can seek and close. We hand it a dup()
 *	of the caller's descriptor so TIFFClose() does not close the FILE* the
 *	caller still owns.
 *
 ****************************************************************************/

#ifdef HAVE_TIFFLIB

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <tiffio.h>

#include "Ilib.h"
#include "IlibP.h"

IError _IReadTIFF ( FILE *fp, IOptions options, IImageP **image_return )
{
  TIFF *tif;
  int fd;
  uint32_t w = 0, h = 0;
  uint32_t *raster;
  size_t n, i;
  IImageP *image;
  unsigned char *d;

  (void) options;

  fd = dup ( fileno ( fp ) );
  if ( fd < 0 )
    return ( ITIFFError );
  tif = TIFFFdOpen ( fd, "ilib", "r" );
  if ( !tif ) {
    close ( fd );
    return ( ITIFFError );
  }

  if ( !TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &w ) ||
       !TIFFGetField ( tif, TIFFTAG_IMAGELENGTH, &h ) || w == 0 || h == 0 ) {
    TIFFClose ( tif );
    return ( ITIFFError );
  }

  n = (size_t) w * h;
  raster = (uint32_t *) _TIFFmalloc ( (tmsize_t) ( n * sizeof ( uint32_t ) ) );
  if ( !raster ) {
    TIFFClose ( tif );
    return ( ITIFFError );
  }
  /* TOPLEFT so raster[row*w + col] matches our top-down row order. */
  if ( !TIFFReadRGBAImageOriented ( tif, w, h, raster, ORIENTATION_TOPLEFT,
         0 ) ) {
    _TIFFfree ( raster );
    TIFFClose ( tif );
    return ( ITIFFError );
  }

  /* Keep the RGB channels (the helper premultiplies any alpha, so we treat
     TIFF as an opaque format and drop it). */
  image = (IImageP *) ICreateImage ( (int) w, (int) h, IOPTION_NONE );
  if ( !image ) {
    _TIFFfree ( raster );
    TIFFClose ( tif );
    return ( ITIFFError );
  }
  d = image->data;
  for ( i = 0; i < n; i++ ) {
    uint32_t px = raster[i];
    d[i * 3] = (unsigned char) TIFFGetR ( px );
    d[i * 3 + 1] = (unsigned char) TIFFGetG ( px );
    d[i * 3 + 2] = (unsigned char) TIFFGetB ( px );
  }

  _TIFFfree ( raster );
  TIFFClose ( tif );
  *image_return = image;
  return ( INoError );
}

IError _IWriteTIFF ( FILE *fp, IImageP *image, IOptions options )
{
  TIFF *tif;
  int fd, ch = (int) image->channels;
  uint32_t row;

  (void) options;

  fd = dup ( fileno ( fp ) );
  if ( fd < 0 )
    return ( IErrorWriting );
  tif = TIFFFdOpen ( fd, "ilib", "w" );
  if ( !tif ) {
    close ( fd );
    return ( IErrorWriting );
  }

  TIFFSetField ( tif, TIFFTAG_IMAGEWIDTH, (uint32_t) image->width );
  TIFFSetField ( tif, TIFFTAG_IMAGELENGTH, (uint32_t) image->height );
  TIFFSetField ( tif, TIFFTAG_SAMPLESPERPIXEL, ch );
  TIFFSetField ( tif, TIFFTAG_BITSPERSAMPLE, 8 );
  TIFFSetField ( tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
  TIFFSetField ( tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
  TIFFSetField ( tif, TIFFTAG_PHOTOMETRIC,
    ( ch == 1 ) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB );
  TIFFSetField ( tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW );
  if ( ch == 4 ) {
    uint16_t extra = EXTRASAMPLE_UNASSALPHA;
    TIFFSetField ( tif, TIFFTAG_EXTRASAMPLES, 1, &extra );
  }
  TIFFSetField ( tif, TIFFTAG_ROWSPERSTRIP,
    TIFFDefaultStripSize ( tif, 0 ) );

  for ( row = 0; row < (uint32_t) image->height; row++ ) {
    if ( TIFFWriteScanline ( tif,
           image->data + (size_t) row * image->width * ch, row, 0 ) < 0 ) {
      TIFFClose ( tif );
      return ( IErrorWriting );
    }
  }

  TIFFClose ( tif );
  return ( INoError );
}

#endif /* HAVE_TIFFLIB */
