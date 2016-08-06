/*

  Dump a font to stdout as an image

  Usage:
    displayfont [options] fontfile > output.pnm

    where options are:

    -gif	output format is GIF
    -png	output format is PNG
    -ppm	output format is PPM/PNM (default)
    -hex	display ascii char numbers in hex
    -dec	display ascii char numbers in decimal (default)

  19-Jul-1999	Added -png option
		Craig Knudsen	cknudsen@radix.net
  12-Apr-1999	Created
		Craig Knudsen	cknudsen@radix.net

***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>

#include <Ilib.h>

#include "courR10.h"	/* font for labeling table */





/*
** Main
*/
int main ( argc, argv )
int argc;
char *argv[];
{
  IImage image = NULL;
  IFont font = NULL, smallfont = NULL;
  char *fontname = NULL;
  int loop;
  int format = IFORMAT_PPM;
  IError ret;
  int width, height;
  int cell_width = 40;
  int cell_height = 40;
  IColor black, white, grey, navy;
  IGC gc;
  int x = 5, y = 5, subx, suby;
  char temp[20];
  unsigned int w, h;
  int usehex = 0;

  /* process command line arguments */
  for ( loop = 1; loop < argc; loop++ ) {
    if ( strcmp ( argv[loop], "-ppm" ) == 0 )
      format = IFORMAT_PPM;
    else if ( strcmp ( argv[loop], "-pnm" ) == 0 )
      format = IFORMAT_PPM;
    else if ( strcmp ( argv[loop], "-gif" ) == 0 ) {
#ifdef HAVE_GIFLIB
      format = IFORMAT_GIF;
#else
      fprintf ( stderr, "GIF not supported (missing giflib).\n" );
      exit ( 1 );
#endif
    }
    else if ( strcmp ( argv[loop], "-png" ) == 0 ) {
#ifdef HAVE_PNGLIB
      format = IFORMAT_PNG;
#else
      fprintf ( stderr, "PNG not supported (missing libpng).\n" );
      exit ( 1 );
#endif
    }
    else if ( strcmp ( argv[loop], "-hex" ) == 0 )
      usehex = 1;
    else if ( strcmp ( argv[loop], "-dec" ) == 0 )
      usehex = 0;
    else if ( *argv[loop] == '-' ) {
      fprintf ( stderr, "%s: unrecognized argument \"%s\"\n",
        argv[0], argv[loop] );
      exit ( 1 );
    }
    else {
      fontname = "myfont";
      ret = ILoadFontFromFile ( fontname, argv[loop], &font );
      if ( ret ) {
        fprintf ( stderr, "Error loading font: %s\n",
          IErrorString ( ret ) );
        exit ( 1 );
      }
    }
  }

  if ( fontname == NULL ) {
    fprintf ( stderr, "Error: no font file specified.\n" );
    fprintf ( stderr, "Usage:\tdisplayfont [-gif|-png|-ppm] [-hex|-dec] bdffile > outputfile\n" );
    exit ( 1 );
  }

  /* load the small font used for labeling */
  ret = ILoadFontFromData ( "smallfont", courR10_font, &smallfont );

  /* create output */
  width = 16 * cell_width + 10;
  height = 16 * cell_height + 10;
  image = ICreateImage ( width, height, IOPTION_NONE );
  black = IAllocColor ( 0, 0, 0 );
  white = IAllocColor ( 255, 255, 255 );
  grey = IAllocColor ( 192, 192, 192 );
  navy = IAllocColor ( 0, 0, 128 );
  gc = ICreateGC ();
  ISetForeground ( gc, white );
  IFillRectangle ( image, gc, 0, 0, width, height );
  ISetForeground ( gc, black );

  for ( loop = 0; loop < 256; loop++ ) {
    if ( loop % 16 == 0 && loop ) {
      y += cell_height;
      x = 5;
    } else if ( loop ) {
      x += cell_width;
    }
    IDrawRectangle ( image, gc, x, y, (unsigned int)cell_width,
      (unsigned int)cell_height );
    if ( usehex )
      sprintf ( temp, "%02X", loop );
    else
      sprintf ( temp, "%d", loop );
    ISetFont ( gc, smallfont );
    ret = ITextDimensions ( gc, smallfont, temp, strlen ( temp ), &w, &h );
    subx = x + ( cell_width - w ) / 2;
    suby = y + h + 2;
    ISetForeground ( gc, navy );
    IFillRectangle ( image, gc, x + 2, y + 2, cell_width - 3,
      h );
    ISetForeground ( gc, white );
    IDrawString ( image, gc, subx, suby - 1, temp, strlen ( temp ) );

    ISetFont ( gc, font );
    ISetForeground ( gc, black );
    sprintf ( temp, "%c", loop );
    ret = ITextDimensions ( gc, font, temp, strlen ( temp ), &w, &h );
    subx = x + ( cell_width - w ) / 2;
    suby = y + cell_height - 6;
    /* draw a baseline */
    ISetForeground ( gc, grey );
    IDrawLine ( image, gc, x + 1, suby, x + cell_width - 1, suby );
    /* draw the letter */
    ISetForeground ( gc, black );
    IDrawString ( image, gc, subx, suby, temp, 1 );
  }

  /*
  ** Write GIF output file.
  */
  IWriteImageFile ( stdout, image, format, IOPTION_INTERLACED );
  IFreeImage ( image );

  /* exit */
  return ( 0 );
}


