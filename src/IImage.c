/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IImage.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	20-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IGetTransparent()
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "Ilib.h"
#define IIncludeFileFormats /* add some static defines */
#include "IlibP.h"


/* Upper bound on total pixels for a single image. Generous for real use
   (~134 megapixels) but bounds the allocation so a malformed file claiming
   enormous dimensions cannot trigger a huge allocation / OOM. */
#define ILIB_MAX_PIXELS ( 1u << 27 )

IImage ICreateImage ( unsigned width, unsigned height, unsigned options )
{
  IImageP *image;
  size_t channels;
  size_t npixels, nbytes;

  /* Greyscale and alpha are mutually exclusive (no greyscale+alpha yet). */
  if ( ( options & IOPTION_GREYSCALE ) && ( options & IOPTION_ALPHA ) )
    return ( NULL );
  if ( options & IOPTION_GREYSCALE )
    channels = 1;
  else if ( options & IOPTION_ALPHA )
    channels = 4;
  else
    channels = 3;

  if ( width == 0 || height == 0 )
    return ( NULL );
  npixels = (size_t) width * (size_t) height; /* size_t: no overflow */
  if ( npixels > ILIB_MAX_PIXELS )
    return ( NULL );
  nbytes = npixels * channels;

  image = (IImageP *) malloc ( sizeof ( IImageP ) );
  if ( !image )
    return ( NULL );
  memset ( image, '\0', sizeof ( IImageP ) );
  image->width = width;
  image->height = height;
  image->channels = (unsigned short) channels;
  if ( options & IOPTION_GREYSCALE )
    image->greyscale = 1;
  if ( channels == 4 )
    image->has_alpha = 1;

  image->data = (unsigned char *) malloc ( nbytes );
  if ( !image->data ) {
    free ( image );
    return ( NULL );
  }
  memset ( image->data, 255, nbytes );

  image->magic = IMAGIC_IMAGE;

  return ( (IImage) image );
}


IError IDuplicateImage ( IImage image, IImage *image_return )
{
  IImageP *i = (IImageP *) image;
  IImageP *ret;

  if ( i ) {
    if ( i->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( i->greyscale )
    *image_return = ICreateImage ( i->width, i->height, IOPTION_GREYSCALE );
  else if ( i->channels == 4 )
    *image_return = ICreateImage ( i->width, i->height, IOPTION_ALPHA );
  else
    *image_return = ICreateImage ( i->width, i->height, IOPTION_NONE );
  ret = (IImageP *) *image_return;
  if ( !ret )
    return ( IInvalidImage );

  memcpy ( ret->data, i->data,
    (size_t) i->width * i->height * ret->channels );
  ret->transparent = i->transparent;
  ret->interlaced = i->interlaced;
  ret->greyscale = i->greyscale;

  return ( INoError );
}


unsigned int IImageHeight ( IImage image )
{
  IImageP *i = (IImageP *) image;

  if ( i ) {
    if ( i->magic != IMAGIC_IMAGE )
      return ( 0 );
  }
  else
    return ( 0 );

  return ( i->height );
}

unsigned int IImageWidth ( IImage image )
{
  IImageP *i = (IImageP *) image;

  if ( i ) {
    if ( i->magic != IMAGIC_IMAGE )
      return ( 0 );
  }
  else
    return ( 0 );

  return ( i->width );
}


IError _IFreeImage ( IImage image )
{
  IImageP *i = (IImageP *) image;

  if ( i ) {
    if ( i->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
    i->magic = 0;
    free ( i->data );
    if ( i->comments )
      free ( i->comments );
    i->comments = NULL;
    free ( i );
  }

  return ( INoError );
}

/* Flatten an RGBA image to a new RGB image by compositing over an opaque white
   background. The output borrows the source comments/transparent pointers; the
   caller must clear ret->comments before freeing it to avoid a double free. */
static IImageP *flatten_rgba ( IImageP *src )
{
  IImageP *dst = (IImageP *) ICreateImage ( src->width, src->height, IOPTION_NONE );
  int n, i;

  if ( !dst )
    return ( NULL );
  n = src->width * src->height;
  for ( i = 0; i < n; i++ ) {
    unsigned int a = src->data[i * 4 + 3];
    unsigned int inv = 255 - a;
    dst->data[i * 3 + 0] =
      (unsigned char) ( ( src->data[i * 4 + 0] * a + 255 * inv + 127 ) / 255 );
    dst->data[i * 3 + 1] =
      (unsigned char) ( ( src->data[i * 4 + 1] * a + 255 * inv + 127 ) / 255 );
    dst->data[i * 3 + 2] =
      (unsigned char) ( ( src->data[i * 4 + 2] * a + 255 * inv + 127 ) / 255 );
  }
  dst->comments = src->comments; /* shared; cleared by caller before free */
  dst->transparent = src->transparent;
  return ( dst );
}

IError IWriteImageFile ( FILE *fp, IImage image, IFileFormat format, IOptions options )
{
  IError ret;
  IImageP *imagep = (IImageP *) image;
  IImageP *flat = NULL;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  /* Most format writers are alpha-unaware; flatten RGBA over white unless the
     target format can write alpha itself (e.g. PNG). */
  if ( imagep->has_alpha && !IFileFormats[format].alpha_ok ) {
    flat = flatten_rgba ( imagep );
    if ( !flat )
      return ( IErrorWriting );
    imagep = flat;
  }

  if ( IFileFormats[format].write_func )
    ret = IFileFormats[format].write_func ( fp, imagep, options );
  else {
    fprintf ( stderr, "IWriteImageFile: %s format write not implemented.\n",
      IFileFormats[format].name );
    ret = IFunctionNotImplemented;
  }

  if ( flat ) {
    flat->comments = NULL; /* shared with the source image; do not free */
    _IFreeImage ( (IImage) flat );
  }

  return ( ret );
}


IError IReadImageFile ( FILE *fp, IFileFormat format, IOptions options, IImage *image_return )
{
  IError ret = INoError;

  if ( format < 0 || format >= INUM_FORMATS ) {
    fprintf ( stderr, "IReadImageFile: invalid format %d\n", format );
    return ( IInvalidFormat );
  }
  else {
    if ( IFileFormats[format].read_func )
      ret = IFileFormats[format].read_func ( fp, options, (IImageP **) image_return );
    else {
      fprintf ( stderr, "IReadImageFile: %s format write not implemented.\n",
        IFileFormats[format].name );
      return ( IFunctionNotImplemented );
    }
  }

  return ( ret );
}


IError ISetComment ( IImage image, char *comments )
{
  IImageP *imagep = (IImageP *) image;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( imagep->comments != NULL )
    free ( imagep->comments );
  if ( comments == NULL )
    imagep->comments = NULL;
  else {
    imagep->comments = (char *) malloc ( strlen ( comments ) + 1 );
    strcpy ( imagep->comments, comments );
  }
  return INoError;
}

IError IGetComment ( IImage image, char **comments )
{
  IImageP *imagep = (IImageP *) image;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  *comments = imagep->comments;
  return INoError;
}


IError ISetTransparent ( IImage image, IColor color )
{
  IImageP *imagep = (IImageP *) image;
  IColorP *colorp;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  colorp = _IGetColor ( color );
  if ( colorp ) {
    if ( colorp->magic != IMAGIC_COLOR )
      return ( IInvalidColor );
  }
  else
    return ( IInvalidColor );

  imagep->transparent = colorp;

  return ( INoError );
}


IError IGetTransparent ( IImage image, IColor *color )
{
  IImageP *imagep = (IImageP *) image;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( imagep->transparent && imagep->transparent->magic == IMAGIC_COLOR ) {
    *color = imagep->transparent->value;
    return ( INoError );
  }
  else {
    return ( INoTransparentColor );
  }
}
