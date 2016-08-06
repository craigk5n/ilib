/*
 * IJPEGC.c
 *
 * Image library
 *
 * Description:
 *	JPEG reading and writing
 *	Most of this code was created from the example.c file provided
 *	in the JPEG distribution.
 *
 *	Get the JPEG distribution at:
 *		ftp://ftp.uu.net/graphics/jpeg/
 *	
 *
 * History:
 *	15-Aug-01	Craig Knudsen	cknudsen@cknudsen.com
 *			Don't let libjpeg call exit() on errors.
 *			Code contributed by Greg McLearn
 *	22-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#ifdef HAVE_JPEGLIB

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <setjmp.h>

#include <jpeglib.h>
#include <jerror.h>

#include "Ilib.h"
#include "IlibP.h"

#define DEFAULT_JPEG_QUALITY	75


struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */
  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * The routine that will replace the standard error_exit method:
 */
METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  /* (*cinfo->err->output_message) (cinfo); */

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}





IError _IWriteJPEG ( fp, image, options )
FILE *fp;
IImageP *image;
IOptions options;
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  int loop;

  /* Step 1: allocate and initialize JPEG compression object */
  cinfo.err = jpeg_std_error ( &jerr );
  jpeg_create_compress ( &cinfo );

  /* Step 2: specify data destination (eg, a file) */
  jpeg_stdio_dest ( &cinfo, fp );

  /* Step 3: set parameters for compression */
  cinfo.image_width = image->width;
  cinfo.image_height = image->height;
  if ( image->greyscale ) {
    cinfo.input_components = 1; /* # of color components per pixel */
    cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */
  } else {
    cinfo.input_components = 3; /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
  }

  jpeg_set_defaults ( &cinfo );
  jpeg_set_quality ( &cinfo, DEFAULT_JPEG_QUALITY, TRUE );

  /* Step 4: Start compressor */
  jpeg_start_compress ( &cinfo, TRUE );

  /* Step 5: while (scan lines remain to be written) */
  for ( loop = 0; loop < image->height; loop++ ) {
    row_pointer[0] = &image->data[loop * image->width * cinfo.input_components];
    (void) jpeg_write_scanlines ( &cinfo, row_pointer, 1 );
  }

  /* Step 6: Finish compression */
  jpeg_finish_compress ( &cinfo );

  /* Step 7: release JPEG compression object */
  jpeg_destroy_compress ( &cinfo );

  return ( INoError );
}





IError _IReadJPEG ( fp, options, image_return )
FILE *fp;
IOptions options;
IImageP **image_return;
{
  IImageP *image;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  JSAMPROW row_pointer[1];
  int loop;
  
  /* We set up the normal JPEG error routines */
  cinfo.err = jpeg_std_error ( &jerr.pub );

  /* Install the new error handler so that we don't get dumped out unceremoniously -- GLM 2000 */
  /* RETURNS IInvalidError on error! (Should really insert a new error enum for it though). */
  jerr.pub.error_exit = my_error_exit;
  if (setjmp (jerr.setjmp_buffer))  {
    /* If we get here, then the JPEG library has signalled an event */
    jpeg_destroy_decompress (&cinfo);
    return IInvalidImage;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress ( &cinfo );

  /* Step 2: specify data source (eg, a file) */
  jpeg_stdio_src ( &cinfo, fp );

  /* Step 3: read file parameters with jpeg_read_header() */
  (void) jpeg_read_header ( &cinfo, TRUE );

  /* Step 5: Start decompressor */
  (void) jpeg_start_decompress ( &cinfo );

  /* allocate Ilib image as either grayscale or rgb */
  image = (IImageP *) ICreateImage ( cinfo.output_width, cinfo.output_height,
    cinfo.output_components == 1 ? IOPTION_GREYSCALE : IOPTION_NONE );
  image->comments = (char *) malloc ( strlen ( IDEFAULT_COMMENT ) + 1 );
  strcpy ( image->comments, IDEFAULT_COMMENT );

  /* Step 6: while (scan lines remain to be read) */
  for ( loop = 0; loop < cinfo.output_height; loop++ ) {
    row_pointer[0] = &image->data[loop * cinfo.output_width *
      cinfo.output_components];
    (void) jpeg_read_scanlines ( &cinfo, row_pointer, 1 );
  }

  /* Step 7: Finish decompression */
  (void) jpeg_finish_decompress ( &cinfo );

  /* Step 8: Release JPEG decompression object */
  jpeg_destroy_decompress ( &cinfo );

  *image_return = image;

  return ( INoError );
}

#endif
