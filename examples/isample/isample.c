/*
 * sample.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * Usage:
 *	sample [-width N] [-height N] [-text inputtext] [-out file]
 *
 * History:
 *	28-Nov-99	Craig Knudsen	cknudsen@radix.net
 *			Added use of IFillArc().
 *	26-Aug-99	Craig Knudsen	cknudsen@radix.net
 *			Added use of named colors.
 *	23-Aug-99	Craig Knudsen	cknudsen@radix.net
 *			Added use of text styles.
 *	26-Jul-99	Craig Knudsen	cknudsen@radix.net
 *			Misc. updates
 *	12-Apr-99	Craig Knudsen	cknudsen@radix.net
 *			Fixed to use real copyright ASCII code
 *			rather than escape.
 *	17-May-98	Craig Knudsen	cknudsen@radix.net
 *			Updated date to 1998
 *			Added call to ITextDimensions to center text.
 *	20-May-96	Craig Knudsen	cknudsen@radix.net
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <Ilib.h>

#define DEFAULT_FONTNAME_1	"helvR24"
#define DEFAULT_FONT_1		helvR24_font
#define DEFAULT_FONTNAME_2	"helvR08"
#define DEFAULT_FONT_2		helvR08_font

/* This will embed the font data within the application so that you can
** distribute the binary by itself.
*/
#include "helvR24.h"
#include "helvR08.h"


int main ( argc, argv )
int argc;
char *argv[];
{
  IImage image;
  IFont largefont, smallfont;
  char *fontname1 = NULL, *fontname2 = NULL;
  int loop;
  unsigned int width = 500, height = 150, text_width = 0, text_height = 0,
    font_height = 0, font_width, sample_width, sample_height;
  int x, y;
  char *outfile = "out.ppm";
  char *sample_text = NULL;
  char *copyright = "@Copyright 2004 Craig Knudsen";
  IFileFormat output_format = IFORMAT_PPM;
  char *infile = NULL;
  FILE *fp;
  IGC gc;
  IError ret;
  IColor topshadow, bottomshadow, textcolor, background, black, white;

  for ( loop = 1; loop < argc; loop++ ) {
    if ( strcmp ( argv[loop], "-w" ) == 0 ||
      strcmp ( argv[loop], "-width" ) == 0 ) {
      if ( ++loop >= argc ) {
        fprintf ( stderr, "-width requires an argument\n" );
        exit ( 1 );
      }
      width = atoi ( argv[loop] );
    }
    else if ( strcmp ( argv[loop], "-h" ) == 0 ||
      strcmp ( argv[loop], "-height" ) == 0 ) {
      if ( ++loop >= argc ) {
        fprintf ( stderr, "-height requires an argument\n" );
        exit ( 1 );
      }
      height = atoi ( argv[loop] );
    }
    else if ( strcmp ( argv[loop], "-font" ) == 0 ) {
      if ( ++loop >= argc ) {
        fprintf ( stderr, "-font requires an argument\n" );
        exit ( 1 );
      }
      fontname1 = argv[loop];
    }
    else if ( strcmp ( argv[loop], "-t" ) == 0 ||
      strcmp ( argv[loop], "-text" ) == 0 ) {
      if ( ++loop >= argc ) {
        fprintf ( stderr, "-text requires an argument\n" );
        exit ( 1 );
      }
      sample_text = argv[loop];
    }
    else if ( strcmp ( argv[loop], "-o" ) == 0 ||
      strcmp ( argv[loop], "-out" ) == 0 ) {
      if ( ++loop >= argc ) {
        fprintf ( stderr, "-out requires an argument\n" );
        exit ( 1 );
      }
      outfile = argv[loop];
    }
    else if ( strcmp ( argv[loop], "-i" ) == 0 ||
      strcmp ( argv[loop], "-infile" ) == 0 ) {
      if ( ++loop >= argc ) {
        fprintf ( stderr, "-infile requires an argument\n" );
        exit ( 1 );
      }
      infile = argv[loop];
    }
  }

  if ( sample_text == NULL ) {
    sample_text = (char *) malloc ( 100 );
    sprintf ( sample_text, "Ilib v%s (%s)\n%s", ILIB_VERSION, ILIB_VERSION_DATE,
      ILIB_URL );
  }
  
  if ( infile ) {
    fp = fopen ( infile, "r" );
    if ( ! fp ) {
      perror ( "Error opening input file:" );
      exit ( 1 );
    }
    if ( ( ret = IReadImageFile ( fp, IFORMAT_PPM, IOPTION_NONE, &image ) ) ) {
      fprintf ( stderr, "Error reading image: %d\n", ret );
      exit ( 1 );
    }
    fclose ( fp );
  }
  else {
    image = ICreateImage ( width, height, IOPTION_NONE );
  }

  gc = ICreateGC ();
  IAllocNamedColor ( "gray", &background );
  ISetBackground ( gc, background );
  IAllocNamedColor ( "lightgrey", &topshadow );
  IAllocNamedColor ( "darkgrey", &bottomshadow );
  IAllocNamedColor ( "yellow", &textcolor );
  IAllocNamedColor ( "black", &black );
  IAllocNamedColor ( "white", &white );

  /* draw top shadow rectangle */
  ISetForeground ( gc, topshadow );
  IFillRectangle ( image, gc, 0, 0, width, height );

  /* draw bottom shadow rectangle */
  ISetForeground ( gc, bottomshadow );
  IFillRectangle ( image, gc, 2, 2, width - 2, height - 2 );

  /* draw background rectangle */
  ISetForeground ( gc, background );
  IFillRectangle ( image, gc, 2, 2, width - 4, height - 4 );

  /* Now the fun part: draw some text */
  if ( fontname1 ) {
    if ( ( ret = ILoadFontFromFile ( fontname1, fontname1, &largefont ) ) ) {
      fprintf ( stderr, "Error (%s) loading font: %s\n",
        IErrorString ( ret ), fontname1 );
      exit ( 1 );
    }
  }
  else {
    fontname1 = DEFAULT_FONTNAME_1;
    if ( ( ret = ILoadFontFromData ( fontname1, DEFAULT_FONT_1, &largefont ) ) ) {
      fprintf ( stderr, "Error (%s) loading font: %s\n", 
        IErrorString ( ret ), fontname1 );
      exit ( 1 );
    }
  }
  fontname2 = DEFAULT_FONTNAME_2;
  if ( ( ret = ILoadFontFromData ( fontname2, DEFAULT_FONT_2, &smallfont ) ) ) {
    fprintf ( stderr, "Error (%s) loading font: %s\n", IErrorString ( ret ),
      fontname2 );
    exit ( 1 );
  }

  ISetFont ( gc, largefont );
  ret = ITextDimensions ( gc, largefont, sample_text, strlen ( sample_text ),
    &text_width, &text_height );
  ret = ITextDimensions ( gc, largefont, "X", 1, &font_width, &font_height );
  x = ( (int)width - (int)text_width ) / 2;
  y = ( ( (int)height - (int)text_height ) / 2 ) + (int)font_height;

  /* draw arc */
  ISetForeground ( gc, topshadow );
  IFillArc ( image, gc, x - 2, y - 2, 20, 30, 90, 270 );
  IFillArc ( image, gc, width - x - 2, y - 2, 20, 30, -90, 90 );
  IFillRectangle ( image, gc, x - 2, y - 30 - 2, width - 2 * x, 60 );
  ISetForeground ( gc, bottomshadow );
  IFillArc ( image, gc, x + 2, y + 2, 20, 30, 90, 270 );
  IFillArc ( image, gc, width - x + 2, y + 2, 20, 30, -90, 90 );
  IFillRectangle ( image, gc, x + 2, y - 30 + 2, width - 2 * x, 60 );
  ISetForeground ( gc, background );
  IFillArc ( image, gc, x, y, 20, 30, 90, 270 );
  IFillArc ( image, gc, width - x, y, 20, 30, -90, 90 );
  IFillRectangle ( image, gc, x, y - 30, width - 2 * x, 60 );

  /* draw text */
  ISetForeground ( gc, textcolor );
  ISetTextStyle ( gc, ITEXT_SHADOWED );
  IDrawString ( image, gc, x, y, sample_text, strlen ( sample_text ) );

  y += text_height;

  /* draw "SAMPLE" from top to bottom on the left side */
  ISetTextStyle ( gc, ITEXT_ETCHED_IN );
  ret = ITextDimensions ( gc, largefont, "SAMPLE", strlen ( "SAMPLE" ),
    &sample_width, &sample_height );
  IDrawStringRotated ( image, gc, 8, ( height - sample_width ) / 2 + 1,
    "SAMPLE", 6, ITEXT_TOP_TO_BOTTOM );

  /* draw "SAMPLE" from bottom to top on the right side */
  IDrawStringRotated ( image, gc,
    width - 6, ( height + sample_width ) / 2 - 1,
    "SAMPLE", strlen ( "SAMPLE" ), ITEXT_BOTTOM_TO_TOP );

  /* draw copyright */
  ISetTextStyle ( gc, ITEXT_SHADOWED );
  ISetFont ( gc, smallfont );
  ISetForeground ( gc, textcolor );
  IDrawString ( image, gc, x, y, copyright, strlen ( copyright ) );

  if ( outfile ) {
    /* determine output file type (ppm/png/gif/etc.) by filename extension */
    ret = IFileType ( outfile, &output_format );
    if ( ret ) {
      fprintf ( stderr, "Output file error: %s\n", IErrorString ( ret ) );
      exit ( 1 );
    }
    /* make sure to include "b" (for binary) for Win32 */
    fp = fopen ( outfile, "wb" );
    if ( ! fp ) {
      perror ( "Cannot open output file: " );
      exit ( 1 );
    }
  }
  else
    fp = stdout;

  /* write output image file */
  IWriteImageFile ( fp, image, output_format, IOPTION_INTERLACED );

  fclose ( fp );

  /* free up resources */
  IFreeFont ( largefont );
  IFreeFont ( smallfont );
  IFreeImage ( image );

  return ( 0 );
}


