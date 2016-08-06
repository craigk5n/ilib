/*

  Report generator for QuakeWorld frag log file

  Usage:
    frag_graph player fraglogfile [fraglogfile ...] > out.gif

  NOTE:
    Player names are case-sensitive!


  18-May-98	Craig Knudsen	cknudsen@radix.net
		Better error handling of command line args
		Don't crash if user not found in logs
  23-May-97	Craig Knudsen	cknudsen@radix.net
		Created


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

#include "helvB18.h"
#include "courR10.h"


#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#define max(a,b) ( (a) > (b) ? (a) : (b) )
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define FRAG	1
#define DEATH	2
#define SUICIDE	3


static void read_file (
#ifndef _NO_PROTO
  char *filename
#endif
);
static void add_stat (
#ifndef _NO_PROTO
  int type
#endif
);
static void generate_gif ();
static int calc_max (
#ifndef _NO_PROTO
  int cur_max
#endif
);



/*
** Define start and stop times.  Use YYMMDDHHMMSS so that we can just
** use strcmp() to compare times.
*/
static char *player = NULL;
static int *stats = NULL;
static int num_stats = 0;	/* total number of frags counted */


static int display_header = TRUE;
static int line_width = 2;	/* for line graphs (can use 1-3) */



/*
** Main
*/
int main ( argc, argv )
int argc;
char *argv[];
{
  int loop;

  stats = (int *) malloc ( 1 );

  for ( loop = 1; loop < argc; loop++ ) {
    if ( *argv[loop] == '-' ) {
      fprintf ( stderr, "%s: unrecognized argument \"%s\"\n",
        argv[0], argv[loop] );
      exit ( 1 );
    }
    else if ( player == NULL ) {
      player = argv[loop];
    }
    else {
      read_file ( argv[loop] );
    }
  }

  if ( player == NULL ) {
    fprintf ( stderr, "Usage: frag_frag playername frag_1.log ...\n" );
    exit ( 1 );
  }

  /* create output */
  generate_gif ();

  /* exit */
  return ( 0 );
}





/****************************************************************************
*
* Read in the data file
*
****************************************************************************/
static void read_file ( filename )
char *filename;
{
  FILE *fp;
  char text[1024], *ptr;

  fp = fopen ( filename, "r" );
  if ( ! fp ) {
    fprintf ( stderr, "Unable to open log file %s\n", filename );
    return;
  }
  while ( fgets ( text, 10240, fp ) ) {
    ptr = strtok ( text, "\\" );
    if ( ! ptr )
      continue;
    if ( strcmp ( player, ptr ) == 0 ) {
      ptr = strtok ( NULL, "\\" );
      if ( ! ptr )
        continue;
      if ( strcmp ( player, ptr ) == 0 )
        add_stat ( SUICIDE );
      else
        add_stat ( FRAG );
    } else {
      ptr = strtok ( NULL, "\\" );
      if ( ! ptr )
        continue;
      if ( strcmp ( player, ptr ) == 0 )
        add_stat ( DEATH );
    }
  }
  fclose ( fp );
}



static void add_stat ( type )
int type;
{
  num_stats++;
  stats = (int *) realloc ( stats, ( num_stats * sizeof ( int * ) ) );
  stats[num_stats-1] = type;
}





/****************************************************************************
*
* Generate the output image
*
****************************************************************************/
#define LEFT_PAD	80
#define TOP_PAD		50
#define RIGHT_PAD	50
#define BOTTOM_PAD	75
#define DATA_WIDTH	20
#define DATA_HEIGHT	200
static void generate_gif ()
{
  int loop;
  char temp[200];
  int x, y, lastx, lasty;
  int r_x, r_y, r_lastx, r_lasty;
  IImage im_out;	/* output image */
  IFont helvB18, courR10;
  IColor red, green, blue, black, grey;
  IGC gc;
  int height, width;
  int max_gibs;
  int gib_step, column;
  int frags = 0, deaths = 0, suicides = 0;
  int r_frags = 0, r_deaths = 0, r_suicides = 0;
  double eff;

  max_gibs = calc_max ( num_stats );
  gib_step = max_gibs / 20;

  width = LEFT_PAD + ( 20 * DATA_WIDTH ) + RIGHT_PAD;
  height = TOP_PAD + DATA_HEIGHT + BOTTOM_PAD;

  /* allocate image */
  im_out = ICreateImage ( width, height, IOPTION_NONE );
  /* first color is background color */
  black = IAllocColor ( 0, 0, 0 );
  grey = IAllocColor ( 192, 192, 192 );
  ISetTransparent ( im_out, grey );
  red = IAllocColor ( 255, 0, 0 );
  green = IAllocColor ( 0, 150, 0 );
  blue = IAllocColor ( 0, 0, 255 );
  gc = ICreateGC ();
  ISetForeground ( gc, grey );
  IFillRectangle ( im_out, gc, 0, 0, width, height );

  /* draw black border */
  ISetForeground ( gc, black );
  IDrawRectangle ( im_out, gc, 0, 0, width - 1, height - 1 );

  /* draw title */
  sprintf ( temp, "Frag Efficiency Graph 1.0: %s", player );
  ILoadFontFromData ( "courR10", courR10_font, &courR10 );
  ILoadFontFromData ( "helvB18", helvB18_font, &helvB18 );
  
  /* Draw title */
  ISetFont ( gc, helvB18 );
  ISetForeground ( gc, grey );
  IDrawString ( im_out, gc, LEFT_PAD + 31, 31, temp, strlen ( temp ) );
  ISetForeground ( gc, black );
  IDrawString ( im_out, gc, LEFT_PAD + 30, 30, temp, strlen ( temp ) );

  /* write "Efficiency" to the left of the y axis */
  ISetFont ( gc, courR10 );
  IDrawString ( im_out, gc, 5, TOP_PAD + DATA_HEIGHT / 2 - 5,
    "Efficiency", strlen ( "Efficiency" ) );

  /* label the y axis  with the top value */
  strcpy ( temp, "100%" );
  IDrawString ( im_out, gc, 5, TOP_PAD + 3, temp, strlen ( temp ) );
  ISetForeground ( gc, blue );
  IDrawLine ( im_out, gc, LEFT_PAD - 3, TOP_PAD, LEFT_PAD, TOP_PAD );

  /* draw x and y axis in blue */
  IDrawLine ( im_out, gc, LEFT_PAD, TOP_PAD, LEFT_PAD, TOP_PAD + DATA_HEIGHT );
  IDrawLine ( im_out, gc, LEFT_PAD, TOP_PAD + DATA_HEIGHT, width - RIGHT_PAD,
    TOP_PAD + DATA_HEIGHT );

  /* draw dashed lines horizontally */
  ISetLineStyle ( gc, ILINE_ON_OFF_DASH );
  for ( loop = 0; loop <= 10; loop++ ) {
    y = TOP_PAD + ( ( DATA_HEIGHT / 10 ) * loop );
    IDrawLine ( im_out, gc, LEFT_PAD, y, width - RIGHT_PAD, y );
  }
  ISetLineStyle ( gc, ILINE_SOLID );

  ISetForeground ( gc, black );
  IDrawString ( im_out, gc, LEFT_PAD + 80, height - BOTTOM_PAD + 22,
    "Frags + Deaths + Suicides", strlen ( "Frags + Deaths + Suicides" ) );

  column = 0;
  for ( loop = 0; loop <= num_stats; loop++ ) {
    switch ( stats[loop] ) {
      case FRAG: frags++; r_frags++; break;
      case DEATH: deaths++; r_deaths++; break;
      case SUICIDE: suicides++; r_suicides++; break;
    }
    if ( gib_step && loop % gib_step == 0 && loop ) {
      column++;
      r_x = x = LEFT_PAD + ( DATA_WIDTH * column );
      eff = ( (double)frags / (double) loop );
      y = TOP_PAD + (int)
        ( (double)( 1.0 - eff ) * (double)DATA_HEIGHT );
      eff = ( (double)r_frags / (double)gib_step );
      r_y = TOP_PAD + (int)
        ( (double)( 1.0 - eff ) * (double)DATA_HEIGHT );
      /* draw the line graph */
      if ( column > 1 ) {
        ISetForeground ( gc, red );
        ISetLineWidth ( gc, line_width );
        IDrawLine ( im_out, gc, lastx, lasty, x, y );
        ISetLineWidth ( gc, 1 );
        ISetForeground ( gc, green );
        ISetLineWidth ( gc, line_width );
        IDrawLine ( im_out, gc, r_lastx, r_lasty, r_x, r_y );
        ISetLineWidth ( gc, 1 );
      }
      /* draw a tic mark */
      ISetForeground ( gc, blue );
      IDrawLine ( im_out, gc, x, TOP_PAD + DATA_HEIGHT, x, 
        TOP_PAD + DATA_HEIGHT + 3 );
      /* label the x axis */
      if ( column % 2 == 0 ) {
        ISetForeground ( gc, black );
        sprintf ( temp, "%d", loop );
        IDrawString ( im_out, gc, x - 5, TOP_PAD + DATA_HEIGHT + 11,
           temp, strlen ( temp ) );
      }
      /* save x and y for the next point */
      lastx = x;
      lasty = y;
      r_lastx = r_x;
      r_lasty = r_y;
      r_deaths = r_frags = r_suicides = 0;
    }
  }

  if ( display_header ) {
    sprintf ( temp, "Total gibs for interval: %d", num_stats );
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, 5, height - 38, temp, strlen ( temp ) );
  }
  ISetForeground ( gc, red );
  IFillRectangle ( im_out, gc, 5, height - 33, 10, 10 );
  sprintf ( temp, "Cumulative efficiency" );
  IDrawString ( im_out, gc, 20, height - 23, temp, strlen ( temp ) );
  ISetForeground ( gc, green );
  IFillRectangle ( im_out, gc, 5, height - 20, 10, 10 );
  sprintf ( temp, "Efficiency over last %d gibs", gib_step );
  IDrawString ( im_out, gc, 20, height - 10, temp, strlen ( temp ) );

  /*
  ** make output interlaced
  */

  /*
  ** Write GIF output file.
  */
  IWriteImageFile ( stdout, im_out, IFORMAT_GIF, IOPTION_INTERLACED );
  IFreeImage ( im_out );
}









/****************************************************************************
*
* Calculate a nice round number to use as the maximum for the y axis.
*
****************************************************************************/
static int calc_max ( cur_max )
int cur_max; 	/* the current max value */
{
  int ret_val;
  int next_round, tens, div_val;

  if ( cur_max == 0 )
    return ( 1 );

  tens = (int) log10 ( (double) cur_max );
  next_round = (int) pow ( (double) 10.0, (double) (tens+1) );
  if ( next_round % 10 == 9 )
    next_round++; // strange behavior under Linux
  div_val = (int) ( next_round / (int) cur_max );
  switch ( div_val ) {
    case 10:
    case 9:
    case 8:
    case 7:
    case 6:
    case 5:
    next_round /= 5;
      break;
    case 4:
      next_round /= 4;
      break;
    case 3:
      next_round = (int) ( 0.4 * (float) next_round );
      break;
    case 2:
      next_round /= 2;
      break;
    case 1:
    case 0:
      break;
  }

  ret_val = (int) ( max ( 5.0, (float) next_round ) );

  if ( ret_val < cur_max )
    ret_val = cur_max;	/* floating point round error */

  return ( ret_val );
}



