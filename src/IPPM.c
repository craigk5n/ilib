/*
 * IPPMC.c
 *
 * Image library
 *
 * Description:
 *	PPM/PGM routines.  PPM is 24-bit color.  PGM is 8-bit grayscale.
 *	If someone asks to write a color image to PGM, it will be written
 *	as PPM.
 *
 * History:
 *	23-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added support for reading PGM.
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>

#include "Ilib.h"
#include "IlibP.h"


IError _IWritePPM ( fp, image, options )
FILE *fp;
IImageP *image;
IOptions options;
{
  int r, c;
  unsigned char *ptr;
  unsigned int red, green, blue;

  if ( options & IOPTION_ASCII ) {
    fprintf ( fp, "P3\n" );
    if ( image->comments )
      fprintf ( fp, "# %s\n", image->comments );
    fprintf ( fp, "%d %d\n255\n", image->width, image->height );
    for ( r = 0; r < image->height; r++ ) {
      for ( c = 0; c < image->width; c++ ) {
        if ( image->greyscale ) {
          ptr = image->data + ( r * image->width ) + c;
          red = green = blue = (unsigned int) *ptr;
        }
        else {
          ptr = image->data + ( r * image->width * 3 ) + ( c * 3 );
          red = (unsigned int) *ptr;
          green = (unsigned int) *( ptr + 1 );
          blue = (unsigned int) *( ptr + 2 );
        }
        fprintf ( fp, "%d %d %d\n", red, green, blue );
      }
    }
  }
  else {
    /* we could write PPM for greyscale, but right PGM since its 1/3 size */
    if ( image->greyscale ) {
      fprintf ( fp, "P5\n" );
      if ( image->comments )
        fprintf ( fp, "# %s\n", image->comments );
      fprintf ( fp, "%d %d\n255\n", image->width, image->height );
      if ( fwrite ( image->data, 1, image->width * image->height, fp ) <= 0 )
        return ( IErrorWriting );
    } else {
      fprintf ( fp, "P6\n" );
      if ( image->comments )
        fprintf ( fp, "# %s\n", image->comments );
      fprintf ( fp, "%d %d\n255\n", image->width, image->height );
      if ( fwrite ( image->data, 3, image->width * image->height, fp ) <= 0 )
        return ( IErrorWriting );
     }
  }

  return ( INoError );
}





IError _IReadPPM ( fp, options, image_return )
FILE *fp;
IOptions options;
IImageP **image_return;
{
  char data[1024];
  IImageP *image;
  int w, h;
  int i;
  unsigned int temp;
  unsigned char *ptr;
  char *p;
  char *comments = NULL;
  int maxcolors = 255;
  int greyscale = 0;

  fgets ( data, 1024, fp );
  if ( strncmp ( data, "P5", 2 ) == 0 ) {
    greyscale = 1;
    options |= IOPTION_GREYSCALE;
  } else if ( strncmp ( data, "P6", 2 ) )
    return ( IFileInvalid );

  fgets ( data, 1024, fp );
  while ( data[0] == '#' ) {
    if ( comments == NULL ) {
      for ( p = data + 1; *p != ' ' && *p != '\0'; p++) ;
      if ( strlen ( p ) ) {
        comments = (char *) malloc ( strlen ( p ) + 1 );
        strcpy ( comments, p );
      }
    }
    fgets ( data, 1024, fp );
  }
  /* should now contain width and height */
  if ( sscanf ( data, "%d %d", &w, &h ) != 2 )
    return ( IFileInvalid );
  fgets ( data, 1024, fp );
  while ( data[0] == '#' )
    fgets ( data, 1024, fp );
  /* should now contain maxcolor value */
  if ( sscanf ( data, "%d", &maxcolors ) != 1 )
    return ( IFileInvalid );

  image = (IImageP *) ICreateImage ( w, h, options );
  if ( comments )
    image->comments = comments;
  else {
    image->comments = (char *) malloc ( strlen ( IDEFAULT_COMMENT ) + 1 );
    strcpy ( image->comments, IDEFAULT_COMMENT );
  }

  if ( greyscale )
    fread ( image->data, 1, w * h, fp );
  else
    fread ( image->data, 1, w * h * 3, fp );

  /* Normalize to 255 if not already */
  if ( maxcolors != 255 ) {
    for ( i = 0; i < ( w * h * 3 ); i++ ) {
      ptr = image->data + i;
      temp = *ptr;
      temp = 255 * temp / maxcolors;
      *ptr = (unsigned char) temp;
    }
  }
  
  *image_return = image;

  return ( INoError );
}

