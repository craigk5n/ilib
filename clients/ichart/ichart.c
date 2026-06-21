/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ichart.c
 *
 * Ilib charting tool (installed as ilib-chart).
 *
 * Description:
 *	Render a chart from CSV data. The first column holds the category labels
 *	(or, for a scatter chart, the x values); each remaining column is a data
 *	series. A header row (unless --no-header) names the series. The chart
 *	type, title, axis labels, size and a few style toggles come from flags.
 *
 *	Usage:  ilib-chart [options] [datafile] outfile
 *	        (datafile defaults to standard input; "-" also means stdin)
 *
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ichart.h>
#include <Ilib.h>

#include "helvR08.h"

#define MAX_SERIES 32
#define MAX_ROWS 8192
#define LINE_MAX 16384

/* A categorical colour palette (Tableau-10-ish) for the data series. */
static IColor palette ( int i )
{
  static const unsigned char rgb[10][3] = {
    { 0x4e, 0x79, 0xa7 }, { 0xf2, 0x8e, 0x2b }, { 0x59, 0xa1, 0x4f },
    { 0xe1, 0x57, 0x59 }, { 0x76, 0xb7, 0xb2 }, { 0xed, 0xc9, 0x48 },
    { 0xb0, 0x7a, 0xa1 }, { 0xff, 0x9d, 0xa7 }, { 0x9c, 0x75, 0x5f },
    { 0xba, 0xb0, 0xac } };
  if ( i < 0 )
    i = 0;
  return ( IAllocColor ( rgb[i % 10][0], rgb[i % 10][1], rgb[i % 10][2] ) );
}

/* strdup is not in ISO C; provide a small equivalent. */
static char *dupstr ( const char *s )
{
  size_t n = strlen ( s ) + 1;
  char *d = (char *) malloc ( n );
  if ( d )
    memcpy ( d, s, n );
  return ( d );
}

/* Trim leading/trailing whitespace in place; returns the trimmed start. */
static char *trim ( char *s )
{
  char *end;
  while ( *s && isspace ( (unsigned char) *s ) )
    s++;
  end = s + strlen ( s );
  while ( end > s && isspace ( (unsigned char) end[-1] ) )
    *--end = '\0';
  return ( s );
}

static IFont load_font ( void )
{
  static const char *ttf[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf", "/Library/Fonts/Arial.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf", NULL };
  IFont font;
  int i;
  for ( i = 0; ttf[i]; i++ )
    if ( ILoadFontFromFileTTF ( "sans", (char *) ttf[i], 11, &font ) ==
         INoError )
      return ( font );
  if ( ILoadFontFromData ( "sans", helvR08_font, &font ) == INoError )
    return ( font );
  return ( NULL );
}

static IChartType parse_type ( const char *s )
{
  if ( strcmp ( s, "line" ) == 0 )
    return ( ICHART_LINE );
  if ( strcmp ( s, "bar" ) == 0 )
    return ( ICHART_BAR );
  if ( strcmp ( s, "pie" ) == 0 )
    return ( ICHART_PIE );
  if ( strcmp ( s, "area" ) == 0 )
    return ( ICHART_AREA );
  if ( strcmp ( s, "scatter" ) == 0 )
    return ( ICHART_SCATTER );
  if ( strcmp ( s, "hbar" ) == 0 )
    return ( ICHART_HBAR );
  if ( strcmp ( s, "donut" ) == 0 )
    return ( ICHART_DONUT );
  fprintf ( stderr,
    "Unknown chart type \"%s\" "
    "(use line|bar|hbar|pie|donut|area|scatter)\n",
    s );
  exit ( 1 );
}

static void usage ( const char *prog )
{
  fprintf ( stderr,
    "Usage: %s [options] [datafile] outfile\n"
    "\n"
    "Render a chart from CSV data (datafile, or standard input). The first\n"
    "column is the category labels (x values for scatter); each remaining\n"
    "column is a data series, named by the header row.\n"
    "\n"
    "Options:\n"
    "  --type T          line|bar|hbar|pie|donut|area|scatter (default line)\n"
    "  --title TEXT      chart title\n"
    "  --xlabel TEXT     x-axis label\n"
    "  --ylabel TEXT     y-axis label\n"
    "  --width N         image width (default 640)\n"
    "  --height N        image height (default 400)\n"
    "  --stacked         stack bar/hbar series\n"
    "  --log             logarithmic value axis\n"
    "  --values          draw value labels\n"
    "  --background NAME  background color (default white)\n"
    "  --no-header       the CSV has no header row\n"
    "  --no-markers      omit point markers on line/area charts\n"
    "  --no-grid         omit gridlines\n"
    "  --no-legend       omit the legend\n"
    "  -h, --help        show this help\n",
    prog );
}

int main ( int argc, char *argv[] )
{
  const char *infile = NULL, *outfile = NULL;
  const char *title = NULL, *xlabel = NULL, *ylabel = NULL, *background = NULL;
  IChartType type = ICHART_LINE;
  int width = 640, height = 400;
  int stacked = 0, logscale = 0, values = 0, header = 1;
  int markers = 1, grid = 1, legend = 1;
  char *cat[MAX_ROWS];
  double val[MAX_SERIES][MAX_ROWS];
  char *sname[MAX_SERIES];
  int nser = 0, nrow = 0, i, s;
  FILE *fp;
  char line[LINE_MAX];
  IChart chart;
  IImage img;
  IFont font;
  IFileFormat fmt = IFORMAT_PNG;
  IError ret;

  for ( i = 0; i < MAX_SERIES; i++ )
    sname[i] = NULL;

  /* Parse arguments. */
  for ( i = 1; i < argc; i++ ) {
    if ( strcmp ( argv[i], "-h" ) == 0 || strcmp ( argv[i], "--help" ) == 0 ) {
      usage ( argv[0] );
      return ( 0 );
    }
    else if ( strcmp ( argv[i], "--type" ) == 0 && i + 1 < argc )
      type = parse_type ( argv[++i] );
    else if ( strcmp ( argv[i], "--title" ) == 0 && i + 1 < argc )
      title = argv[++i];
    else if ( strcmp ( argv[i], "--xlabel" ) == 0 && i + 1 < argc )
      xlabel = argv[++i];
    else if ( strcmp ( argv[i], "--ylabel" ) == 0 && i + 1 < argc )
      ylabel = argv[++i];
    else if ( strcmp ( argv[i], "--width" ) == 0 && i + 1 < argc )
      width = atoi ( argv[++i] );
    else if ( strcmp ( argv[i], "--height" ) == 0 && i + 1 < argc )
      height = atoi ( argv[++i] );
    else if ( strcmp ( argv[i], "--background" ) == 0 && i + 1 < argc )
      background = argv[++i];
    else if ( strcmp ( argv[i], "--stacked" ) == 0 )
      stacked = 1;
    else if ( strcmp ( argv[i], "--log" ) == 0 )
      logscale = 1;
    else if ( strcmp ( argv[i], "--values" ) == 0 )
      values = 1;
    else if ( strcmp ( argv[i], "--no-header" ) == 0 )
      header = 0;
    else if ( strcmp ( argv[i], "--no-markers" ) == 0 )
      markers = 0;
    else if ( strcmp ( argv[i], "--no-grid" ) == 0 )
      grid = 0;
    else if ( strcmp ( argv[i], "--no-legend" ) == 0 )
      legend = 0;
    else if ( argv[i][0] == '-' && argv[i][1] != '\0' &&
              strcmp ( argv[i], "-" ) != 0 ) {
      fprintf ( stderr, "Unknown option: %s\n", argv[i] );
      usage ( argv[0] );
      return ( 1 );
    }
    else if ( infile == NULL )
      infile = argv[i];
    else
      outfile = argv[i];
  }

  /* With a single positional it is the output; input is then stdin. */
  if ( outfile == NULL ) {
    outfile = infile;
    infile = NULL;
  }
  if ( outfile == NULL ) {
    fprintf ( stderr, "No output file specified.\n" );
    usage ( argv[0] );
    return ( 1 );
  }

  fp = ( infile && strcmp ( infile, "-" ) != 0 ) ? fopen ( infile, "r" )
                                                 : stdin;
  if ( !fp ) {
    perror ( "open datafile" );
    return ( 1 );
  }

  /* Parse CSV. */
  while ( fgets ( line, sizeof ( line ), fp ) ) {
    char *p = trim ( line ), *tok;
    int col = 0;
    if ( *p == '\0' || *p == '#' )
      continue;
    if ( header ) { /* header row: series names in columns 1.. */
      for ( tok = strtok ( p, "," ); tok; tok = strtok ( NULL, "," ) ) {
        char *t = trim ( tok );
        if ( col >= 1 && col - 1 < MAX_SERIES )
          sname[col - 1] = dupstr ( t );
        col++;
      }
      header = 0;
      continue;
    }
    if ( nrow >= MAX_ROWS )
      break;
    for ( tok = strtok ( p, "," ); tok; tok = strtok ( NULL, "," ) ) {
      char *t = trim ( tok );
      if ( col == 0 )
        cat[nrow] = dupstr ( t );
      else if ( col - 1 < MAX_SERIES )
        val[col - 1][nrow] = atof ( t );
      col++;
    }
    if ( col - 1 > nser )
      nser = col - 1; /* widest data row sets the series count */
    nrow++;
  }
  if ( fp != stdin )
    fclose ( fp );

  if ( nrow == 0 || nser == 0 ) {
    fprintf ( stderr, "No data parsed from input.\n" );
    return ( 1 );
  }

  /* Build the chart. */
  chart = ICreateChart ( type, (unsigned int) width, (unsigned int) height );
  if ( !chart ) {
    fprintf ( stderr, "Could not create chart.\n" );
    return ( 1 );
  }
  font = load_font ();
  if ( font )
    IChartSetFont ( chart, font );
  if ( title )
    IChartSetTitle ( chart, title );
  if ( xlabel || ylabel )
    IChartSetAxisLabels ( chart, xlabel, ylabel );
  if ( stacked )
    IChartSetStacked ( chart, 1 );
  if ( logscale )
    IChartSetLogScale ( chart, 1 );
  if ( values )
    IChartSetValueLabels ( chart, 1 );
  if ( !markers )
    IChartSetMarkers ( chart, 0 );
  if ( !grid )
    IChartSetGrid ( chart, 0 );
  if ( !legend )
    IChartSetLegend ( chart, 0 );
  if ( background ) {
    IColor bg;
    if ( IAllocNamedColor ( (char *) background, &bg ) == INoError )
      IChartSetBackground ( chart, bg );
  }

  if ( type == ICHART_SCATTER ) {
    double *xv = (double *) malloc ( (size_t) nrow * sizeof ( double ) );
    if ( !xv ) {
      fprintf ( stderr, "Out of memory.\n" );
      return ( 1 );
    }
    for ( i = 0; i < nrow; i++ )
      xv[i] = atof ( cat[i] );
    for ( s = 0; s < nser; s++ )
      IChartAddXYSeries ( chart, sname[s], xv, val[s], nrow, palette ( s ) );
    free ( xv );
  }
  else {
    IChartSetCategories ( chart, (const char **) cat, nrow );
    for ( s = 0; s < nser; s++ )
      IChartAddSeries ( chart, sname[s], val[s], nrow, palette ( s ) );
  }

  img = IChartRender ( chart );
  if ( !img ) {
    fprintf ( stderr, "Chart rendering failed.\n" );
    return ( 1 );
  }

  IFileType ( (char *) outfile, &fmt );
  fp = fopen ( outfile, "wb" );
  if ( !fp ) {
    perror ( "open output" );
    return ( 1 );
  }
  ret = IWriteImageFile ( fp, img, fmt, IOPTION_NONE );
  fclose ( fp );
  if ( ret != INoError )
    fprintf ( stderr, "Error writing image: %s\n", IErrorString ( ret ) );

  IFreeImage ( img );
  IFreeChart ( chart );
  if ( font ) {
    IFreeFont ( font );
  }
  for ( i = 0; i < nrow; i++ )
    free ( cat[i] );
  for ( s = 0; s < nser; s++ )
    free ( sname[s] );

  return ( ret == INoError ? 0 : 1 );
}
