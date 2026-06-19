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
#define IIncludeFileFormats	/* add some static defines */
#include "IlibP.h"


/* Upper bound on total pixels for a single image. Generous for real use
   (~134 megapixels) but bounds the allocation so a malformed file claiming
   enormous dimensions cannot trigger a huge allocation / OOM. */
#define ILIB_MAX_PIXELS ( 1u << 27 )

IImage ICreateImage ( unsigned width, unsigned height, unsigned options )
{
  IImageP *image;
  size_t channels = ( options & IOPTION_GREYSCALE ) ? 1 : 3;
  size_t npixels, nbytes;

  if ( width == 0 || height == 0 )
    return ( NULL );
  npixels = (size_t) width * (size_t) height;     /* size_t: no overflow */
  if ( npixels > ILIB_MAX_PIXELS )
    return ( NULL );
  nbytes = npixels * channels;

  image = (IImageP *) malloc ( sizeof ( IImageP ) );
  if ( ! image )
    return ( NULL );
  memset ( image, '\0', sizeof ( IImageP ) );
  image->width = width;
  image->height = height;
  if ( options & IOPTION_GREYSCALE )
    image->greyscale = 1;

  image->data = (unsigned char *) malloc ( nbytes );
  if ( ! image->data ) {
    free ( image );
    return ( NULL );
  }
  memset ( image->data, 255, nbytes );

  image->magic = IMAGIC_IMAGE;

  return ( (IImage)image );
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
  else
    *image_return = ICreateImage ( i->width, i->height, IOPTION_NONE );
  ret = (IImageP *)*image_return;

  if ( i->greyscale )
    memcpy ( ret->data, i->data, i->width * i->height );
  else
    memcpy ( ret->data, i->data, i->width * i->height * 3 );
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

IError IWriteImageFile ( FILE *fp, IImage image, IFileFormat format, IOptions options )
{
  IError ret;
  IImageP *imagep = (IImageP *)image;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( IFileFormats[format].write_func )
    ret = IFileFormats[format].write_func ( fp, imagep, options );
  else {
    fprintf ( stderr, "IWriteImageFile: %s format write not implemented.\n",
      IFileFormats[format].name );
    ret = IFunctionNotImplemented;
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
      ret = IFileFormats[format].read_func ( fp, options, (IImageP **)image_return );
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
  IImageP *imagep = (IImageP *)image;

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
  IImageP *imagep = (IImageP *)image;

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
  IImageP *imagep = (IImageP *)image;
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
  IImageP *imagep = (IImageP *)image;

  if ( imagep ) {
    if ( imagep->magic != IMAGIC_IMAGE )
      return ( IInvalidImage );
  }
  else
    return ( IInvalidImage );

  if ( imagep->transparent && imagep->transparent->magic == IMAGIC_COLOR ) {
    *color = imagep->transparent->value;
    return ( INoError );
  } else {
    return ( INoTransparentColor );
  }
}


