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
#include <memory.h>

#include "Ilib.h"
#define IIncludeFileFormats	/* add some static defines */
#include "IlibP.h"


IImage ICreateImage ( width, height, options )
unsigned width, height, options;
{
  IImageP *image;

  image = (IImageP *) malloc ( sizeof ( IImageP ) );
  memset ( image, '\0', sizeof ( IImageP ) );
  image->width = width;
  image->height = height;
  if ( options & IOPTION_GREYSCALE ) {
    image->data = (unsigned char *) malloc ( width * height );
    memset ( image->data, 255, width * height );
    image->greyscale = 1;
  }
  else {
    image->data = (unsigned char *) malloc ( width * height * 3 );
    memset ( image->data, 255, width * height * 3 );
  }

  image->magic = IMAGIC_IMAGE;

  return ( (IImage)image );
}


IError IDuplicateImage ( image, image_return )
IImage image;
IImage *image_return;
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


unsigned int IImageHeight ( image )
IImage image;
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

unsigned int IImageWidth ( image )
IImage image;
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


IError _IFreeImage ( image )
IImage image;
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

IError IWriteImageFile ( fp, image, format, options )
FILE *fp;
IImage image;
IFileFormat format;
IOptions options;
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
  else
    fprintf ( stderr, "IWriteImageFile: %s format write not implemented.\n",
      IFileFormats[format].name );

  return ( ret );
}



IError IReadImageFile ( fp, format, options, image_return )
FILE *fp;
IFileFormat format;
IOptions options;
IImage *image_return;
{
  IError ret;

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


IError ISetComment ( image, comments )
IImage image;
char *comments;
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

IError IGetComment ( image, comments )
IImage image;
char **comments;
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



IError ISetTransparent ( image, color )
IImage image;
IColor color;
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



IError IGetTransparent ( image, color )
IImage image;
IColor *color;
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


