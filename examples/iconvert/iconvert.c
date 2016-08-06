/*
 * sample.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	23-Jul-99	Craig Knudsen   cknudsen@radix.net
 *			Changed "r" to "rb" for fopen()
 *	17-May-98	Craig Knudsen	cknudsen@radix.net
 *			Created
 *	20-May-96	Craig Knudsen	cknudsen@radix.net
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <Ilib.h>

int main ( argc, argv )
int argc;
char *argv[];
{
  IImage image;
  char *outfile = NULL;
  IFileFormat input_format = IFORMAT_PPM;
  IFileFormat output_format = IFORMAT_PPM;
  char *infile = NULL;
  FILE *fp;
  IError ret;
  int loop;

  for ( loop = 1; loop < argc; loop++ ) {
    if ( infile == NULL ) {
      infile = argv[loop];
    }
    else if ( outfile == NULL ) {
      outfile = argv[loop];
    }
  }

  if ( ! infile )
    fprintf ( stderr, "No infile specified.  Reading from stdin.\n" );
  if ( ! outfile ) {
    outfile = "out.ppm";
    fprintf ( stderr, "No outfile specified.  Writing to %s.\n", outfile );
  }
  
  /* try and determine file types by extension */
  if ( infile ) {
    ret = IFileType ( infile, &input_format );
    if ( ret ) {
      fprintf ( stderr, "Input file error: %s\n", IErrorString ( ret ) );
      exit ( 1 );
    }
  }
  if ( outfile ) {
    ret = IFileType ( outfile, &output_format );
    if ( ret ) {
      fprintf ( stderr, "Output file error: %s\n", IErrorString ( ret ) );
      fprintf ( stderr, "Using PPM format.\n" );
    }
  }

  if ( infile ) {
    fp = fopen ( infile, "rb" );
    if ( ! fp ) {
      perror ( "Error opening input file:" );
      exit ( 1 );
    }
  }
  else
    fp = stdin;

  if ( ( ret = IReadImageFile ( fp, input_format, IOPTION_NONE, &image ) ) ) {
    fprintf ( stderr, "Error reading image: %s\n", IErrorString ( ret ) );
    exit ( 1 );
  }
  if ( infile )
    fclose ( fp );

  if ( outfile ) {
    fp = fopen ( outfile, "wb" );
    if ( ! fp ) {
      perror ( "Cannot open output file: " );
      exit ( 1 );
    }
  }
  else
    fp = stdout;

  IWriteImageFile ( fp, image, output_format, IOPTION_INTERLACED );

  if ( outfile )
    fclose ( fp );

  return ( 0 );
}


