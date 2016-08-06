/*
 * Build an image that contains an index of a bunch of images.
 * 
 *
 * Usage:
 *    index [-wN] [-hN] [-f] [-s] [-h] outimage inimage1 ...
 *
 *	-wN	sets the icon width, Example: -w120
 *	-HN	sets the icon height, Example: -h120
 *	-f	go ahead and overwrite the output if it already exists
 *	-s	silent
 *	-html f	specify an output file to write an HTML client-side image map
 *
 *	File format types will be determined by their filename
 *	extension (".jpg", ".gif", ".png", etc.)
 *
 * Note:
 *	You can use '-' for the outimage to write to standard output:
 *	  ./index - file1.jpg file2.jpg file3.jpg > index.jpg
 *	Note: you can only use JPEG for output this way.
 *
 * History:
 *	09-Dec-1999	Craig Knudsen	cknudsen@radix.net
 *			Display help if no arguments are given
 *	28-Sep-1999	Craig Knudsen	cknudsen@radix.net
 *			Fixed bug where cols was not init to 0
 *	26-Jul-1999	Craig Knudsen	cknudsen@radix.net
 *			Added 3D look
 *			Added -html option to write HTML image maps
 *	23-Jul-1999	Craig Knudsen	cknudsen@radix.net
 *			Created
 *
 **************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Ilib.h>

#include "helvR08.h"


#define TOP_SHADOW		232
#define BOTTOM_SHADOW		112
#define BACKGROUND		192



static char *get_basename ( infile )
char *infile;
{
  char *ptr;

  for ( ptr = infile + strlen ( infile ) - 1;
    *ptr != '/' && *ptr != '\\' && ptr != infile; ptr-- )
    ;

  if ( *ptr == '/' || *ptr == '\\' )
    ptr++;
  return ( ptr );
}


static void print_usage ()
{
  printf ( "Usage:\n  index [options] outfile infile1 infile2 ...\n" );
  printf ( "\nOptions:\n" );
  printf ( "\t-h       show this help information\n" );
  printf ( "\t-f       overwrite outfile if it already exists\n" );
  printf ( "\t-wN      use pixel width of N for icons\n" );
  printf ( "\t-HN      use pixel height of N for icons\n" );
  printf ( "\t-s       run silently (no output to stdout)\n" );
  printf ( "\t-html f  specify an output file to write an HTML client-side image map\n" );
  exit ( 0 );
}


/*
** Main
*/
int main ( argc, argv )
int argc;
char *argv[];
{
  int loop;
  int force = 0;
  int silent = 0;
  int iconW = 80;
  int iconH = 60;
  int thisW, thisH;
  char *outfile = NULL;
  char *infiles[1024];
  char *filename;
  int len;
  unsigned int strw;
  int ninfiles = 0;
  FILE *fp, *in_fp, *map_fp = NULL;
  IFileFormat format;
  int border = 5;
  int textspace = 12;
  int rows, cols, row, col;
  int w, h, x, y;
  IImage image, in_image;
  IFont font;
  IGC gc;
  IColor black, bg, ts, bs;
  IError ret;
  int scale;
  struct stat buf;

  for ( loop = 1; loop < argc; loop++ ) {
    if ( strcmp ( argv[loop], "-h" ) == 0 ||
      strcmp ( argv[loop], "-help" ) == 0 ||
      strcmp ( argv[loop], "--help" ) == 0 ) {
      print_usage ();
    }
    else if ( strcmp ( argv[loop], "-html" ) == 0 ) {
      map_fp = fopen ( argv[++loop], "w" );
      if ( map_fp == NULL ) {
        fprintf ( stderr, "Error: could not write map file \"%s\"\n",
          argv[loop] );
      }
    }
    else if ( strncmp ( argv[loop], "-w", 2 ) == 0 &&
      strlen ( argv[loop] ) > 2 ) {
      iconW = atoi ( argv[loop] + 2 );
      if ( iconW <= 0 ) {
        fprintf ( stderr, "Invalid width setting \"%s\"\n", argv[loop] );
        exit ( 1 );
      }
    }
    else if ( strncmp ( argv[loop], "-h", 2 ) == 0 &&
      strlen ( argv[loop] ) > 2 ) {
      iconH = atoi ( argv[loop] + 2 );
      if ( iconH <= 0 ) {
        fprintf ( stderr, "Invalid height setting \"%s\"\n", argv[loop] );
        exit ( 1 );
      }
    }
    else if ( strncmp ( argv[loop], "-f", 2 ) == 0 )
      force = 1;
    else if ( strncmp ( argv[loop], "-s", 2 ) == 0 )
      silent = 1;
    else if ( outfile == NULL ) {
      outfile = argv[loop];
      if ( strcmp ( outfile, "-" ) == 0 ) {
        fp = stdin;
      } else {
        if ( stat ( outfile, &buf ) == 0 && ! force ) {
          fprintf ( stderr, "Error: output file %s already exists.\n",
            outfile );
          fprintf ( stderr, "Delete it or use the -f option.\n" );
          exit ( 1 );
        }
        fp = fopen ( outfile, "wb" );
        if ( fp == NULL ) {
          fprintf ( stderr, "Error writing to %s\n", outfile );
          exit ( 1 );
        }
      }
    }
    else {
      infiles[ninfiles++] = argv[loop];
    }
  }

  if ( outfile == NULL ) {
    print_usage ();
  }

  /* load font */
  if ( ( ret = ILoadFontFromData ( "helvR08", helvR08_font, &font ) ) ) {
    fprintf ( stderr, "Error (%s) loading font: helvR08\n",
      IErrorString ( ret ) );
    exit ( 1 );
  }

  gc = ICreateGC ();

  /* determine size of output image */
  cols = 0;
  while ( cols * cols < ninfiles )
    cols++;
  rows = ninfiles / cols;
  if ( ninfiles > ( rows * cols ) )
    rows++;
  w = border + cols * ( iconW + border );
  h = border + rows * ( iconH + border + textspace );

  image = ICreateImage ( w, h, IOPTION_NONE );

  gc = ICreateGC ();
  black = IAllocColor ( 0, 0, 0 );
  bg = IAllocColor (  BACKGROUND, BACKGROUND, BACKGROUND );
  ts = IAllocColor (  TOP_SHADOW, TOP_SHADOW, TOP_SHADOW );
  bs = IAllocColor (  BOTTOM_SHADOW, BOTTOM_SHADOW, BOTTOM_SHADOW );
  ISetForeground ( gc, bg );
  IFillRectangle ( image, gc, 0, 0, w, h );

  ISetForeground ( gc, black );
  ISetFont ( gc, font );

  if ( map_fp != NULL ) {
    fprintf ( map_fp,
      "<html><head><title>Image Index</title></head>\n" );
    fprintf ( map_fp, "<body bgcolor=\"#%02x%02x%02x\"><h2>Image Index</h2>\n",
      BACKGROUND, BACKGROUND, BACKGROUND );
    fprintf ( map_fp, "<map name=\"image_index\">\n" );
  }
  for ( row = col = loop = 0; loop < ninfiles; loop++ ) {
    filename = get_basename ( infiles[loop] );
    ret = IFileType ( infiles[loop], &format );
    if ( ret ) {
      fprintf ( stderr, "Input file error: %s\n", IErrorString ( ret ) );
      exit ( 1 );
    }
    in_fp = fopen ( infiles[loop], "rb" );
    if ( in_fp == NULL ) {
      fprintf ( stderr, "Error opening file %s\n", infiles[loop] );
      continue;
    }
    if ( ! silent )
      printf ( "Reading %s.\n", infiles[loop] );
    if ( ( ret = IReadImageFile ( in_fp, format, IOPTION_NONE, &in_image ) ) ) {
      fprintf ( stderr, "Error reading image %s: %s\n", infiles[loop],
        IErrorString ( ret ) );
      exit ( 1 );
    }
    x = border + col * ( iconW + border );
    y = border + row * ( iconH + border + textspace );
    /* determine scaled size.  maintain proportions */
    scale = 0;
    do {
      scale++;
      thisW = IImageWidth ( in_image ) / scale;
      thisH = IImageHeight ( in_image ) / scale;
    } while ( thisW > iconW || thisH > iconH );
    /*
    if ( ! silent )
      printf ( "\tCopying to (%d,%d) as %dx%d (scale=1:%d)\n",
        x, y, thisW, thisH, scale );
    */
    ISetForeground ( gc, ts );
    IDrawLine ( image, gc, x - 1, y - 1, x + iconW + 1, y - 1 );
    IDrawLine ( image, gc, x - 1, y - 1, x - 1, y + iconH + 1 );
    ISetForeground ( gc, bs );
    IDrawLine ( image, gc, x - 1, y + iconH + 1, x + iconW + 1, y + iconH + 1 );
    IDrawLine ( image, gc, x + iconW + 1, y - 1, x + iconW + 1, y + iconH + 1 );
    ISetForeground ( gc, black );
    ret = ICopyImageScaled ( in_image, image, gc,
      0, 0, IImageWidth ( in_image ), IImageHeight ( in_image ),
      x + ( iconW - thisW ) / 2, y + ( iconH - thisH ) / 2, thisW, thisH );
    /* don't write more text than will fit under the image. */
    len = strlen ( filename );
    ret = ITextWidth ( gc, font, filename, len, &strw );
    while ( strw > iconW && len > 0 ) {
      ret = ITextWidth ( gc, font, filename, --len, &strw );
    }
    IDrawString ( image, gc, x, y + iconH + textspace - 2,
      filename, len );
    if ( map_fp != NULL )
      fprintf ( map_fp,
        "<area shape=\"rect\" coords=\"%d,%d,%d,%d\" href=\"%s\">\n",
        x, y, x + iconW, y + iconH, infiles[loop] );
    IFreeImage ( in_image );
    col++;
    if ( col >= cols ) {
      col = 0;
      row++;
    }
  }
  if ( map_fp != NULL ) {
    fprintf ( map_fp,
      "<img src=\"%s\" width=\"%d\" height=\"%d\" usemap=\"#image_index\" border=\"0\">\n",
      outfile, IImageWidth ( image ), IImageHeight ( image ) );
    fprintf ( map_fp, "</map>\n" );
    fprintf ( map_fp,
      "<p><font size=\"-1\">Generated with <A HREF=\"%s\">Ilib</A>.</font>\n",
      ILIB_URL );
    fprintf ( map_fp, "</body></html>\n" );
  }

  if ( strcmp ( outfile, "-" ) == 0 )
    format = IFORMAT_JPEG;
  else
    ret = IFileType ( outfile, &format );
  fp = fopen ( outfile, "wb" );
  if ( ! fp ) {
    fprintf ( stderr, "Error writing %s.\n", outfile );
    exit ( 1 );
  }
  if ( ! silent )
    printf ( "Writing %s.\n", outfile );
  IWriteImageFile ( fp, image, format, IOPTION_NONE );

  if ( strcmp ( outfile, "-" ) )
    fclose ( fp );

  /* exit */
  return ( 0 );
}


