/*
 * IPPMC.c
 *
 * Image library
 *
 * Description:
 *	PGM routines.  PGM is 8-bit grayscale.
 *
 * History:
 *	23-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#include "Ilib.h"
#include "IlibP.h"


IError _IWritePGM ( fp, image, options )
FILE *fp;
IImageP *image;
IOptions options;
{
  int r, c;
  unsigned char *ptr;
  unsigned int val, red, green, blue;

  if ( options & IOPTION_ASCII ) {
    fprintf ( fp, "P2\n" );
    if ( image->comments )
      fprintf ( fp, "# %s\n", image->comments );
    fprintf ( fp, "%d %d\n255\n", image->width, image->height );
    for ( r = 0; r < image->height; r++ ) {
      for ( c = 0; c < image->width; c++ ) {
        if ( image->greyscale ) {
          ptr = image->data + ( r * image->width ) + c;
          val = (unsigned int) *ptr;
        }
        else {
          ptr = image->data + ( r * image->width * 3 ) + ( c * 3 );
          red = (unsigned int) *ptr;
          green = (unsigned int) *( ptr + 1 );
          blue = (unsigned int) *( ptr + 2 );
          val = ( red + green + blue ) / 3;
        }
        fprintf ( fp, "%d\n", val );
      }
    }
  }
  else {
    fprintf ( fp, "P5\n" );
    if ( image->comments )
      fprintf ( fp, "# %s\n", image->comments );
    fprintf ( fp, "%d %d\n255\n", image->width, image->height );
    if ( image->greyscale ) {
      if ( fwrite ( image->data, 1, image->width * image->height, fp ) <= 0 )
        return ( IErrorWriting );
    } else {
      for ( r = 0; r < image->height; r++ ) {
        for ( c = 0; c < image->width; c++ ) {
          ptr = image->data + ( r * image->width * 3 ) + ( c * 3 );
          red = (unsigned int) *ptr;
          green = (unsigned int) *( ptr + 1 );
          blue = (unsigned int) *( ptr + 2 );
          val = ( red + green + blue ) / 3;
          if ( fwrite ( &val, 1, 1, fp ) <= 0 )
            return ( IErrorWriting );
        }
      }
    }
  }

  return ( INoError );
}




/* NOTE: There is no _IReadPGM().  We use _ReadPPM() instead since it
 *       can handle both PPM and PGM.
 */


