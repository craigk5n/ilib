/*
 * IPNG.c
 *
 * Image library
 *
 * Description:
 *	Implementation of read and write for PNG.
 *	Requires PNG library available at:
 *		http://www.cdrom.com/pub/png/pngcode.html
 *
 * Comments:
 *	Still need to implement 8-bit colormap support. (Currently
 *	all images are written out as truecolor which makes them a little
 *	larger.)
 *	Transparency, Alpha channels and interlacing are not yet
 *	supported when writing files.
 *
 * History:
 * 	01-Apr-00	Jim Winstead	jimw@trainedmonkey.com
 * 			Fixed PNG output
 *	19-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#ifdef HAVE_PNGLIB

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#include <png.h>

#include "Ilib.h"
#include "IlibP.h"


IError _IWritePNG ( fp, image, options )
FILE *fp;
IImageP *image;
IOptions options;
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_text comment;
  int row;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
    (png_voidp) NULL, (png_error_ptr) NULL, (png_error_ptr) NULL );
  if ( png_ptr == NULL )
    return IPNGError;

  info_ptr = png_create_info_struct ( png_ptr );
  if ( setjmp ( png_jmpbuf ( png_ptr ) ) )
    return IPNGError;

  png_init_io ( png_ptr, fp );

  /* TODO: convert to 8-bit palette image instead of always using
  ** full-color images.
  */

  /* set header info -- the 8 means 8 bits per color, not 8 bits per pixel */
  png_set_IHDR ( png_ptr, info_ptr, image->width, image->height,
    8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

  /* set comment */
  if ( image->comments != NULL ) {
    comment.compression = PNG_TEXT_COMPRESSION_NONE;
    comment.key = "Comment";
    comment.text = image->comments;
    png_set_text ( png_ptr, info_ptr, &comment, 1 );
  }

  /* write header */
  png_write_info ( png_ptr, info_ptr );

  /* let libpng take care of bit-depth conversions */
  /*png_set_packing ( png_ptr );*/

  /* write out image, one row at a time
  ** we have to convert to PNG's 16-bit format.
  */
  for ( row = 0; row < image->height; row++ ) {
    png_write_row ( png_ptr, (image->data + image->width * row * 3));
  }
  png_write_end ( png_ptr, info_ptr );
  fflush ( fp );
  png_destroy_write_struct ( &png_ptr, (png_infopp)NULL );

  return ( INoError );
}





IError _IReadPNG ( fp, options, image_return )
FILE *fp;
IOptions options;
IImageP **image_return;
{
  IImageP *image = NULL;
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, row;
  png_bytep row_pointer;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
    (png_voidp) NULL, (png_error_ptr) NULL, (png_error_ptr) NULL );
  if ( png_ptr == NULL )
    return IPNGError;

  /* Allocate/initialize the memory for image information.  REQUIRED. */
  info_ptr = png_create_info_struct ( png_ptr );
  if ( info_ptr == NULL ) {
    png_destroy_read_struct ( &png_ptr, (png_infopp)NULL, (png_infopp)NULL );
    return ( IPNGError );
  }
 
  /* setup default error handling */
  if ( setjmp ( png_jmpbuf ( png_ptr ) ) ) {
    /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_read_struct ( &png_ptr, &info_ptr, (png_infopp)NULL );
    return ( IPNGError );
  }

  png_init_io ( png_ptr, fp );

  /* read header */
  png_read_info ( png_ptr, info_ptr );
  png_get_IHDR ( png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
    &interlace_type, NULL, NULL );

  image = (IImageP *) ICreateImage ( width, height, IOPTION_NONE );

  /* Expand paletted colors into true RGB triplets */
  if ( color_type == PNG_COLOR_TYPE_PALETTE )
    png_set_expand ( png_ptr );

  /* Strip alpha bytes from the input data without combining with the
  ** background (not recommended).
  */
  png_set_strip_alpha ( png_ptr );

  /* Read the image one line at a time.  That way we don't have to
  ** malloc two images.
  */
  row_pointer = (png_bytep) png_malloc ( png_ptr,
      png_get_rowbytes ( png_ptr, info_ptr ) );

  row_pointer = (png_bytep) malloc ( image->width * 3 );
  for ( row = 0; row < height; row++ ) {
    png_read_rows ( png_ptr, &row_pointer, NULL, 1 );
    memcpy ( image->data + ( row * image->width * 3 ),
      row_pointer, ( image->width * 3 ) );
  }
  free ( row_pointer );

  /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
  png_read_end ( png_ptr, info_ptr );

  /* clean up */
  png_destroy_read_struct ( &png_ptr, &info_ptr, (png_infopp)NULL );

  *image_return = image;

  return ( INoError );
}


#endif /* HAVE_PNGLIB */

