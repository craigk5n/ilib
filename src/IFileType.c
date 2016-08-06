/*
 * IFileType.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	01-Apr-00	Jim Winstead	imw@trainedmonkey.com
 *			Added BMP.
 *	19-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added PNG.
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



IError IFileType ( file, format_return )
char *file;
IFileFormat *format_return;
{
  char *tmp, *ptr;
  IError ret = INoError;

  *format_return = IFORMAT_PPM; /* default */

  tmp = (char *) malloc ( strlen ( file ) + 1 );
  strcpy ( tmp, file );

  for ( ptr = tmp; *ptr != '\0'; ptr++ )
    *ptr = tolower ( *ptr );

  /* find the last '.' */
  for ( ptr = tmp + strlen ( tmp ) - 1; ( ptr != tmp ) && ( *ptr != '.' );
    ptr-- ) ;
  if ( *ptr == '.' ) {
    ptr++;
    if ( strcmp ( ptr, "gif" ) == 0 ) {
      *format_return = IFORMAT_GIF;
#ifndef HAVE_GIFLIB
      ret = INoGIFSupport;
#endif
    } else if ( strcmp ( ptr, "ppm" ) == 0 )
      *format_return = IFORMAT_PPM;
    else if ( strcmp ( ptr, "pgm" ) == 0 )
      *format_return = IFORMAT_PGM;
    else if ( strcmp ( ptr, "pbm" ) == 0 )
      *format_return = IFORMAT_PBM;
    else if ( strcmp ( ptr, "xpm" ) == 0 )
      *format_return = IFORMAT_XPM;
    else if ( strcmp ( ptr, "xbm" ) == 0 )
      *format_return = IFORMAT_XBM;
    else if ( strcmp ( ptr, "png" ) == 0 )
      *format_return = IFORMAT_PNG;
    else if ( strcmp ( ptr, "jpeg" ) == 0 )
      *format_return = IFORMAT_JPEG;
    else if ( strcmp ( ptr, "jpg" ) == 0 )
      *format_return = IFORMAT_JPEG;
    else if ( strcmp ( ptr, "bmp" ) == 0 )
      *format_return = IFORMAT_BMP;
    else
      ret = IInvalidFormat;
  } else
    ret = IInvalidFormat;

  free ( tmp );

  return ( ret );
}


