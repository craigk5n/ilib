/*

  Report generator for WWW access.log file

  Usage:
    httpgraph [options] [-tod|-dom|-dow|-moy] accesslogfile [accesslogfile ...]

    Default action is to generate a day of the month usage graph to stdout.

    -tod		Display a time of day report indicating what
			hours of the day get the most usage.
			[THIS IS THE DEFAULT]
    -dom		Display a day of month report indicating what
			days of the month get the most usage.
    -dow		Display a day of week report indicating what
			days of the week get the most usage.
    -moy		Displays month report indicating usage by month

    options		what it does
    ----------------	------------------------------------------------
    -all		Use all data [default]
    -today		Use data from today only
    -yesterday		Use data from yesterday only
    -lastweek		Use data from the Mon-Sun week prior to this one
    -thisweek		Use data for this Mon-Sun week
    -thismonth		Use data for the the current month
    -lastmonth		Use data for the month prior to this one
    -bar		Use a bar graph instead of a line graph.
    -line		Use line graph [default]
    -nohdr		Do not display title and summary at bottom

  21-Sep-94	Craig Knudsen	cknudsen@radix.net
		Created
  29-May-96	Craig Knudsen	cknudsen@radix.net
  29-May-96	Craig Knudsen	cknudsen@radix.net
		Converted from gd library to Ilib.


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


static void read_file (
#ifndef _NO_PROTO
  char *filename
#endif
);
static void add_time (
#ifndef _NO_PROTO
  char *text
#endif
);
static int is_in_time_range (
#ifndef _NO_PROTO
  char *text
#endif
);
static void set_times_today ();
static void set_times_yesterday ();
static void set_times_last_week ();
static void set_times_this_week ();
static void set_times_this_month ();
static void set_times_last_month ();
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
static char start_time[20];	/* in YYMMDDHHMMSS format */
static char stop_time[20];	/* in YYMMDDHHMMSS format */
static char pretty_start[30];	/* in human readable format */
static char pretty_stop[30];	/* in human readable format */
static char *months[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};
static char *wdays[] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};
static int days_in_month[] =
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static int days_in_lmonth[] =
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static int tod[24];	/* count by time of day */
static int dow[7];	/* couny by day of week */
static int dom[31];	/* count by day of month */
static int moy[12];	/* count by month */

#define SUNDAY		0
#define MONDAY		1
#define TUESDAY		2
#define WEDNESDAY	3
#define THURSDAY	4
#define FRIDAY		5
#define SATURDAY	6

static int total = 0;	/* total number of requests counted */

typedef enum {
  DAY_OF_MONTH,
  DAY_OF_WEEK,
  TIME_OF_DAY,
  MONTH_OF_YEAR
} OutputType;

typedef enum {
  BAR,
  LINE
} GraphType;

static OutputType output_type = DAY_OF_MONTH;
static GraphType graph_type = LINE;
static int display_header = TRUE;
static int line_width = 2;	/* for line graphs (can use 1-3) */

/* String to use in place of "Retrievals" */
static char *retrievals = "Retrievals";



/*
** Main
*/
int main ( argc, argv )
int argc;
char *argv[];
{
  int loop;

  /* default is to use all data */
  start_time[0] = stop_time[0] = '\0';

  for ( loop = 0; loop < 24; loop++ )
    tod[loop] = 0;
  for ( loop = 0; loop < 7; loop++ )
    dow[loop] = 0;
  for ( loop = 0; loop < 31; loop++ )
    dom[loop] = 0;
  for ( loop = 0; loop < 12; loop++ )
    moy[loop] = 0;

  /* process command line arguments */
  for ( loop = 1; loop < argc; loop++ ) {
    if ( strcmp ( argv[loop], "-bar" ) == 0 )
      graph_type = BAR;
    else if ( strcmp ( argv[loop], "-line" ) == 0 )
      graph_type = LINE;
    else if ( strcmp ( argv[loop], "-tod" ) == 0 )
      output_type = TIME_OF_DAY;
    else if ( strcmp ( argv[loop], "-dom" ) == 0 )
      output_type = DAY_OF_MONTH;
    else if ( strcmp ( argv[loop], "-dow" ) == 0 )
      output_type = DAY_OF_WEEK;
    else if ( strcmp ( argv[loop], "-moy" ) == 0 )
      output_type = MONTH_OF_YEAR;
    else if ( strcmp ( argv[loop], "-today" ) == 0 )
      set_times_today ();
    else if ( strcmp ( argv[loop], "-yesterday" ) == 0 )
      set_times_yesterday ();
    else if ( strcmp ( argv[loop], "-lastweek" ) == 0 )
      set_times_last_week ();
    else if ( strcmp ( argv[loop], "-thisweek" ) == 0 )
      set_times_this_week ();
    else if ( strcmp ( argv[loop], "-thismonth" ) == 0 )
      set_times_this_month ();
    else if ( strcmp ( argv[loop], "-lastmonth" ) == 0 )
      set_times_last_month ();
    else if ( strcmp ( argv[loop], "-noheader" ) == 0 ||
      strcmp ( argv[loop], "-nohdr" ) == 0 )
      display_header = FALSE;
    else if ( strcmp ( argv[loop], "-rstring" ) == 0 ) {
      if ( argv[++loop] == NULL ) {
        fprintf ( stderr, "%s : -rstring requires another parameter\n",
          argv[0] );
        exit ( 1 );
      }
      retrievals = argv[loop];
    }
    else if ( *argv[loop] == '-' ) {
      fprintf ( stderr, "%s: unrecognized argument \"%s\"\n",
        argv[0], argv[loop] );
      exit ( 1 );
    }
    else {
      read_file ( argv[loop] );
    }
  }

  /* create output */
  generate_gif ();

  /* exit */
  return ( 0 );
}


/****************************************************************************
*
* Set the time range variables so that we look at data from today only.
*
****************************************************************************/
static void set_times_today ()
{
  time_t now;
  struct tm *tm;

  time ( &now );

  tm = localtime ( &now );
  sprintf ( start_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 0, 0, 0 );
  sprintf ( pretty_start, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    0, 0, 0 );

  sprintf ( stop_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 23, 59, 59 );
  sprintf ( pretty_stop, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    23, 59, 59 );
}




/****************************************************************************
*
* Set the time range variables so that we look at data from yesterday only.
*
****************************************************************************/
static void set_times_yesterday ()
{
  time_t now, yest;
  struct tm *tm;

  time ( &now );
  yest = now - ( 24 * 3600 );

  tm = localtime ( &yest );
  sprintf ( start_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 0, 0, 0 );
  sprintf ( pretty_start, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    0, 0, 0 );
  sprintf ( stop_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 23, 59, 59 );
  sprintf ( pretty_stop, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    23, 59, 59 );
}




/****************************************************************************
*
* Set the time range variables so that we look at data from last week
* where the week starts on Monday and finishes on Sunday.
*
****************************************************************************/
static void set_times_last_week ()
{
  time_t now, lastweek, lastweekstop;
  struct tm *tm;

  time ( &now );
  tm = localtime ( &now );
  switch ( tm->tm_wday ) {
    case MONDAY:
      lastweek = now - ( 7 * ( 24 * 3600 ) ); break;
    case TUESDAY:
      lastweek = now - ( 8 * ( 24 * 3600 ) ); break;
    case WEDNESDAY:
      lastweek = now - ( 9 * ( 24 * 3600 ) ); break;
    case THURSDAY:
      lastweek = now - ( 10 * ( 24 * 3600 ) ); break;
    case FRIDAY:
      lastweek = now - ( 11 * ( 24 * 3600 ) ); break;
    case SATURDAY:
      lastweek = now - ( 12 * ( 24 * 3600 ) ); break;
    case SUNDAY:
      lastweek = now - ( 13 * ( 24 * 3600 ) ); break;
  }

  tm = localtime ( &lastweek );
  sprintf ( start_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 0, 0, 0 );
  sprintf ( pretty_start, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    0, 0, 0 );

  lastweekstop = lastweek + ( 7 * 24 * 3600 );
  tm = localtime ( &lastweekstop );
  sprintf ( stop_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 23, 59, 59 );
  sprintf ( pretty_stop, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    23, 59, 59 );
}




/****************************************************************************
*
* Set the time range variables so that we look at data from this week
* where the week starts on Monday and finishes on Sunday.
*
****************************************************************************/
static void set_times_this_week ()
{
  time_t now, thisweek, thisweekstop;
  struct tm *tm;

  time ( &now );
  tm = localtime ( &now );
  switch ( tm->tm_wday ) {
    case MONDAY:
      thisweek = now - ( 0 * ( 24 * 3600 ) ); break;
    case TUESDAY:
      thisweek = now - ( 1 * ( 24 * 3600 ) ); break;
    case WEDNESDAY:
      thisweek = now - ( 2 * ( 24 * 3600 ) ); break;
    case THURSDAY:
      thisweek = now - ( 3 * ( 24 * 3600 ) ); break;
    case FRIDAY:
      thisweek = now - ( 4 * ( 24 * 3600 ) ); break;
    case SATURDAY:
      thisweek = now - ( 5 * ( 24 * 3600 ) ); break;
    case SUNDAY:
      thisweek = now - ( 6 * ( 24 * 3600 ) ); break;
  }

  tm = localtime ( &thisweek );
  sprintf ( start_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 0, 0, 0 );
  sprintf ( pretty_start, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    0, 0, 0 );

  thisweekstop = thisweek + ( 7 * 24 * 3600 );
  tm = localtime ( &thisweekstop );
  sprintf ( stop_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 23, 59, 59 );
  sprintf ( pretty_stop, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    23, 59, 59 );
}





/****************************************************************************
*
* Set the time range variables so that we look at data from this month only.
*
****************************************************************************/
static void set_times_this_month ()
{
  time_t now, first, last;
  struct tm *tm;

  /* Figure out the first of the month */
  time ( &now );
  tm = localtime ( &now );
  first = now - ( ( tm->tm_mday - 1 ) * ( 24 * 3600 ) );

  /* start of month */
  tm = localtime ( &first );
  sprintf ( start_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 0, 0, 0 );
  sprintf ( pretty_start, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    0, 0, 0 );

  /* end of month */
  if ( tm->tm_year % 4 == 0 ) {
    /* leap year */
    last = first + ( ( days_in_lmonth[tm->tm_mon] - 1 ) * 24 * 3600 );
  }
  else {
    last = first + ( ( days_in_month[tm->tm_mon] - 1 ) * 24 * 3600 );
  }
  tm = localtime ( &last );
  sprintf ( stop_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 23, 59, 59 );
  sprintf ( pretty_stop, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    23, 59, 59 );
}




/****************************************************************************
*
* Set the time range variables so that we look at data from the month
* prior to the current month.
*
****************************************************************************/
static void set_times_last_month ()
{
  time_t now, first, last;
  struct tm *tm;
  int prev_month;

  /* Figure out the 1st of the current month */
  time ( &now );
  tm = localtime ( &now );
  first = now - ( ( tm->tm_mday - 1 ) * ( 24 * 3600 ) );

  /* move to the 1st of previous month */
  prev_month = tm->tm_mon - 1;
  if ( prev_month < 0 )
    prev_month = 12;
  if ( tm->tm_year % 4 == 0 ) {
    /* leap year */
    first -= days_in_lmonth[prev_month] * 24 * 3600;
  }
  else {
    first -= days_in_month[prev_month] * 24 * 3600;
  }

  /* start */
  tm = localtime ( &first );
  sprintf ( start_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 0, 0, 0 );
  sprintf ( pretty_start, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    0, 0, 0 );

  /* end of month */
  if ( tm->tm_year % 4 == 0 ) {
    /* leap year */
    last = first + ( ( days_in_lmonth[tm->tm_mon] - 1 ) * 24 * 3600 );
  }
  else {
    last = first + ( ( days_in_month[tm->tm_mon] - 1 ) * 24 * 3600 );
  }
  tm = localtime ( &last );
  sprintf ( stop_time, "%02d%02d%02d%02d%02d%02d",
    tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, 23, 59, 59 );
  sprintf ( pretty_stop, "%s %02d %s %d %02d:%02d:%02d",
    wdays[tm->tm_wday], tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
    23, 59, 59 );
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
  char text[10240];

  fp = fopen ( filename, "r" );
  if ( ! fp ) {
    fprintf ( stderr, "Unable to open log file %s\n", filename );
    return;
  }
  while ( fgets ( text, 10240, fp ) ) {
    if ( is_in_time_range ( text ) ) {
      /* get hour of day, day of week, and day of month */
      add_time ( text );
      total++;
    }
  }
  fclose ( fp );
}



/*
** Add the time information so we can report time of day, day of month.
*/
static void add_time ( text )
char *text;
{
  char *ptr;
  char temp[10];
  int int1, loop, month, year, day, hour, weekday, y, m, d;
  static int dlook[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

  temp[2] = '\0';

  /* first get the day of month */
  ptr = strstr ( text, "[" );
  day = -1;
  if ( ! ptr ) {
    /* very unexpected.  (but expect the unexpected, right?) */
    return;
  }
  strncpy ( temp, ptr + 1, 2 );
  if ( isdigit ( temp[0] ) && isdigit ( temp[1] ) ) {
    int1 = atoi ( temp );
    if ( int1 >= 1 && int1 <= 31 )
      day = int1 -1;
  }
  if ( day < 0 )
    return;

  /* now get the month */
  temp[3] = '\0';
  month = -1;
  strncpy ( temp, ptr + 4, 3 );
  for ( loop = 0; loop < 12; loop++ ) {
    if ( strcmp ( months[loop], temp ) == 0 ) {
      month = loop; /* remember: Jan=0 */
      break;
    }
  }
  if ( month < 0 )
    return;

  /* now get the year */
  temp[4] = '\0';
  year = -1;
  strncpy ( temp, ptr + 8, 4 );
  if ( isdigit ( temp[0] ) && isdigit ( temp[1] ) &&
    isdigit ( temp[2] ) && isdigit ( temp[3] ) ) {
    int1 = atoi ( temp );
    if ( int1 >= 1900 && int1 <= 9999 )
      year = int1;
  }
  if ( year < 0 )
    return;

  /* now get the hour of day */
  temp[2] = '\0';
  strncpy ( temp, ptr + 13, 2 );
  if ( isdigit ( temp[0] ) && isdigit ( temp[1] ) ) {
    int1 = atoi ( temp );
    if ( int1 >= 0 && int1 <= 23 )
      hour = int1;
  }
  if ( hour < 0 )
    return;

  /* determine day of week from a nifty algorithm */
  m = month + 1;
  y = year;
  d = day + 1;
  if ( m < 3 )
    y--;
  weekday = ( y + ( y/4 ) - ( y/100 ) + ( y/400 ) + dlook[m-1] + d ) % 7;

/*
  fprintf ( stderr, "%sdow=%s\tmonth=%s\tdom=%d\ttod=%d\n",
    text, wdays[weekday], months[month], day + 1, hour );
*/
  dow[weekday]++;
  tod[hour]++;
  moy[month]++;
  dom[day]++;
}





/****************************************************************************
*
* Generate the output image
*
****************************************************************************/
#define LEFT_PAD	80
#define TOP_PAD		50
#define RIGHT_PAD	50
#define BOTTOM_PAD	55
#define DATA_WIDTH	15
#define DATA_HEIGHT	200
static void generate_gif ()
{
  int loop;
  char temp[200];
  int maxval;
  int lastday;
  int x, y, lastx, lasty;
  IImage im_out;	/* output image */
  IFont helvB18, courR10;
  IColor red, blue, black, grey;
  IGC gc;
  int height, width;

  /*
  ** Display day of month data
  */
  if ( output_type == DAY_OF_MONTH ) {
    /* get max value and last day we have data for */
    for ( maxval = 0, loop = 0; loop < 31; loop++ ) {
      maxval = max ( maxval, dom[loop] );
      if ( dom[loop] )
        lastday = loop;
    }
    maxval = calc_max ( maxval );
    /* Create graph output */
    width = LEFT_PAD + ( ( lastday + 1 ) * DATA_WIDTH ) + RIGHT_PAD;
    width = max ( width, 500 );
    height = TOP_PAD + DATA_HEIGHT + BOTTOM_PAD;
  }
  else if ( output_type == TIME_OF_DAY ) {
    /* get max value */
    for ( maxval = 0, loop = 0; loop < 24; loop++ ) {
      maxval = max ( maxval, tod[loop] );
    }
    maxval = calc_max ( maxval );
    /* Create graph output */
    width = LEFT_PAD + ( 24 * DATA_WIDTH ) + RIGHT_PAD;
    height = TOP_PAD + DATA_HEIGHT + BOTTOM_PAD;
  }
  else if ( output_type == DAY_OF_WEEK ) {
    /* get max value */
    for ( maxval = 0, loop = 0; loop < 7; loop++ ) {
      maxval = max ( maxval, dow[loop] );
    }
    maxval = calc_max ( maxval );
    /* Create graph output */
    width = LEFT_PAD + ( 7 * DATA_WIDTH * 3 ) + RIGHT_PAD;
    height = TOP_PAD + DATA_HEIGHT + BOTTOM_PAD;
  }
  else if ( output_type == MONTH_OF_YEAR ) {
    /* get max value */
    for ( maxval = 0, loop = 0; loop < 12; loop++ ) {
      maxval = max ( maxval, moy[loop] );
    }
    maxval = calc_max ( maxval );
    /* Create graph output */
    width = LEFT_PAD + ( 24 * DATA_WIDTH ) + RIGHT_PAD;
    width = max ( width, 500 );
    height = TOP_PAD + DATA_HEIGHT + BOTTOM_PAD;
  }

  /* allocate image */
  im_out = ICreateImage ( width, height, IOPTION_NONE );
  /* first color is background color */
  black = IAllocColor ( 0, 0, 0 );
  grey = IAllocColor ( 192, 192, 192 );
  ISetTransparent ( im_out, grey );
  red = IAllocColor ( 255, 0, 0 );
  blue = IAllocColor ( 0, 0, 255 );
  gc = ICreateGC ();
  ISetForeground ( gc, grey );
  IFillRectangle ( im_out, gc, 0, 0, width, height );

  /* draw black border */
  ISetForeground ( gc, black );
  IDrawRectangle ( im_out, gc, 0, 0, width - 1, height - 1 );

  /* draw title */
  if ( display_header ) {
    sprintf ( temp, "WebReport 1.0" );
  }
  ILoadFontFromData ( "courR10", courR10_font, &courR10 );
  ILoadFontFromData ( "helvB18", helvB18_font, &helvB18 );
  
  /* Draw title */
  ISetFont ( gc, helvB18 );
  ISetForeground ( gc, grey );
  IDrawString ( im_out, gc, LEFT_PAD + 81, 31, temp, strlen ( temp ) );
  ISetForeground ( gc, black );
  IDrawString ( im_out, gc, LEFT_PAD + 80, 30, temp, strlen ( temp ) );

  /* write "Retrievals" to the left of the y axis */
  ISetFont ( gc, courR10 );
  IDrawString ( im_out, gc, 5, TOP_PAD + DATA_HEIGHT / 2 - 5,
    retrievals, strlen ( retrievals ) );

  /* label the y axis  with the top value */
  sprintf ( temp, "%12d", maxval );
  IDrawString ( im_out, gc, 5, TOP_PAD + 3, temp, strlen ( temp ) );
  ISetForeground ( gc, blue );
  IDrawLine ( im_out, gc, LEFT_PAD - 3, TOP_PAD, LEFT_PAD, TOP_PAD );

  /* draw x and y axis in blue */
  IDrawLine ( im_out, gc, LEFT_PAD, TOP_PAD, LEFT_PAD, TOP_PAD + DATA_HEIGHT );
  if ( graph_type == LINE )
    IDrawLine ( im_out, gc, LEFT_PAD, TOP_PAD + DATA_HEIGHT, width - RIGHT_PAD,
      TOP_PAD + DATA_HEIGHT );
  else
    IDrawLine ( im_out, gc, LEFT_PAD, TOP_PAD + DATA_HEIGHT,
      width - RIGHT_PAD + ( DATA_WIDTH / 2 ),
      TOP_PAD + DATA_HEIGHT );

  /* draw dashed lines horizontally */
  ISetLineStyle ( gc, ILINE_ON_OFF_DASH );
  for ( loop = 0; loop <= 4; loop++ ) {
    y = TOP_PAD + ( ( DATA_HEIGHT / 5 ) * loop );
    if ( graph_type == LINE )
      IDrawLine ( im_out, gc, LEFT_PAD, y, width - RIGHT_PAD, y );
    else
      IDrawLine ( im_out, gc, LEFT_PAD, y,
        width - RIGHT_PAD + ( DATA_WIDTH / 2 ), y );
  }
  ISetLineStyle ( gc, ILINE_SOLID );

  if ( output_type == DAY_OF_MONTH ) {
    /* label this as day of month on x axis */
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, LEFT_PAD + 80, height - 35,
      "Day of the Month", strlen ( "Day of the Month" ) );
    /* now draw a line for each point */
    lastx = LEFT_PAD + DATA_WIDTH;
    lasty = TOP_PAD + (int)
      ( ( (double)(maxval - dom[0]) / (double)maxval )
       * (double)DATA_HEIGHT );
    if ( graph_type == BAR ) {
      ISetForeground ( gc, red );
      IFillRectangle ( im_out, gc, lastx - ( DATA_WIDTH / 2 ) + 1, lasty,
        DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - lasty );
    }
    /* draw a tic mark */
    ISetForeground ( gc, blue );
    IDrawLine ( im_out, gc, lastx, TOP_PAD + DATA_HEIGHT, lastx, 
      TOP_PAD + DATA_HEIGHT + 3 );
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, lastx - 2, TOP_PAD + DATA_HEIGHT + 11,
      "1", 1 );
    for ( loop = 1; loop <= lastday; loop++ ) {
      x = LEFT_PAD + ( ( loop + 1 ) * DATA_WIDTH );
      y = TOP_PAD + (int)
        ( ( (double)(maxval - dom[loop]) / (double)maxval )
         * (double)DATA_HEIGHT );
      /* draw the line graph */
      ISetForeground ( gc, red );
      if ( graph_type == LINE ) {
        ISetLineWidth ( gc, line_width );
        IDrawLine ( im_out, gc, lastx, lasty, x, y );
        ISetLineWidth ( gc, 1 );
      }
      else
        IFillRectangle ( im_out, gc, x - ( DATA_WIDTH / 2 ) + 1, y,
          DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - y );
      /* draw a tic mark */
      ISetForeground ( gc, blue );
      IDrawLine ( im_out, gc, x, TOP_PAD + DATA_HEIGHT, x, 
        TOP_PAD + DATA_HEIGHT + 3 );
      /* label the x axis */
      ISetForeground ( gc, black );
      sprintf ( temp, "%d", loop + 1 );
      if ( loop < 9 )
        IDrawString ( im_out, gc, x - 2, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      else
        IDrawString ( im_out, gc, x - 5, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      /* save x and y for the next point */
      lastx = x;
      lasty = y;
    }
  }

  else if ( output_type == TIME_OF_DAY ) {
    /* label this as time of day on x axis */
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, LEFT_PAD + 80, height - 35,
      "Hour of the Day", strlen ( "Hour of the Day" ) );
    /* now draw a line for each point */
    lastx = LEFT_PAD + DATA_WIDTH;
    lasty = TOP_PAD + (int)
      ( ( (double)(maxval - tod[0]) / (double)maxval )
       * (double)DATA_HEIGHT );
    if ( graph_type == BAR ) {
      ISetForeground ( gc, red );
      IFillRectangle ( im_out, gc, lastx - ( DATA_WIDTH / 2 ) + 1, lasty,
        DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - lasty );
    }
    /* draw a tic mark */
    ISetForeground ( gc, blue );
    IDrawLine ( im_out, gc, lastx, TOP_PAD + DATA_HEIGHT, lastx, 
      TOP_PAD + DATA_HEIGHT + 3 );
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, lastx - 2, TOP_PAD + DATA_HEIGHT + 11,
      "1", 1 );
    for ( loop = 1; loop < 24; loop++ ) {
      x = LEFT_PAD + ( ( loop + 1 ) * DATA_WIDTH );
      y = TOP_PAD + (int)
        ( ( (double)(maxval - tod[loop]) / (double)maxval )
         * (double)DATA_HEIGHT );
      /* draw the line graph */
      ISetForeground ( gc, red );
      if ( graph_type == LINE ) {
        ISetLineWidth ( gc, line_width );
        IDrawLine ( im_out, gc, lastx, lasty, x, y );
        ISetLineWidth ( gc, 1 );
      }
      else
        IFillRectangle ( im_out, gc, x - ( DATA_WIDTH / 2 ) + 1, y,
          DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - y );
      /* draw a tic mark */
      ISetForeground ( gc, blue );
      IDrawLine ( im_out, gc, x, TOP_PAD + DATA_HEIGHT, x, 
        TOP_PAD + DATA_HEIGHT + 3 );
      /* label the x axis */
      sprintf ( temp, "%d", loop + 1 );
      ISetForeground ( gc, black );
      if ( loop < 9 )
        IDrawString ( im_out, gc, x - 2, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      else
        IDrawString ( im_out, gc, x - 5, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      /* save x and y for the next point */
      lastx = x;
      lasty = y;
    }
  }

  else if ( output_type == DAY_OF_WEEK ) {
    /* label this as time of day on x axis */
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, LEFT_PAD + 80, height - 35,
      "Day of Week", strlen ( "Day of Week" ) );
    /* now draw a line for each point */
    lastx = LEFT_PAD + DATA_WIDTH * 3;
    lasty = TOP_PAD + (int)
      ( ( (double)(maxval - tod[0]) / (double)maxval )
       * (double)DATA_HEIGHT );
    if ( graph_type == BAR ) {
      ISetForeground ( gc, red );
      IFillRectangle ( im_out, gc, lastx - ( DATA_WIDTH / 2 ) + 1, lasty,
        DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - lasty );
    }
    /* draw a tic mark */
    ISetForeground ( gc, blue );
    IDrawLine ( im_out, gc, lastx, TOP_PAD + DATA_HEIGHT, lastx, 
      TOP_PAD + DATA_HEIGHT + 3 );
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, lastx - 2, TOP_PAD + DATA_HEIGHT + 11,
      "Sun", 3 );
    for ( loop = 1; loop < 7; loop++ ) {
      x = LEFT_PAD + ( ( loop + 1 ) * DATA_WIDTH  * 3 );
      y = TOP_PAD + (int)
        ( ( (double)(maxval - dow[loop]) / (double)maxval )
         * (double)DATA_HEIGHT );
      /* draw the line graph */
      ISetForeground ( gc, red );
      if ( graph_type == LINE ) {
        ISetLineWidth ( gc, line_width );
        IDrawLine ( im_out, gc, lastx, lasty, x, y );
        ISetLineWidth ( gc, 1 );
      }
      else
        IFillRectangle ( im_out, gc, x - ( DATA_WIDTH / 2 ) + 1, y,
          DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - y );
      /* draw a tic mark */
      ISetForeground ( gc, blue );
      IDrawLine ( im_out, gc, x, TOP_PAD + DATA_HEIGHT, x, 
        TOP_PAD + DATA_HEIGHT + 3 );
      /* label the x axis */
      sprintf ( temp, "%s", wdays[loop] );
      ISetForeground ( gc, black );
      if ( loop < 9 )
        IDrawString ( im_out, gc, x - 9, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      else
        IDrawString ( im_out, gc, x - 12, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      /* save x and y for the next point */
      lastx = x;
      lasty = y;
    }
  }

  else if ( output_type == MONTH_OF_YEAR ) {
    /* label this as time of day on x axis */
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, LEFT_PAD + 80, height - 35,
      "Month of Year", strlen ( "Month of Year" ) );
    /* now draw a line for each point */
    lastx = LEFT_PAD + DATA_WIDTH * 2;
    lasty = TOP_PAD + (int)
      ( ( (double)(maxval - moy[0]) / (double)maxval )
       * (double)DATA_HEIGHT );
    if ( graph_type == BAR ) {
      ISetForeground ( gc, red );
      IFillRectangle ( im_out, gc, lastx - ( DATA_WIDTH / 2 ) + 1, lasty,
        DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - lasty );
    }
      /* draw a tic mark */
    ISetForeground ( gc, blue );
    IDrawLine ( im_out, gc, lastx, TOP_PAD + DATA_HEIGHT, lastx, 
      TOP_PAD + DATA_HEIGHT + 3 );
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, lastx - 2, TOP_PAD + DATA_HEIGHT + 11,
      months[0], strlen ( months[0] ) );
    for ( loop = 1; loop < 12; loop++ ) {
      x = LEFT_PAD + ( ( loop + 1 ) * DATA_WIDTH  * 2 );
      y = TOP_PAD + (int)
        ( ( (double)(maxval - moy[loop]) / (double)maxval )
         * (double)DATA_HEIGHT );
      /* draw the line graph */
      ISetForeground ( gc, red );
      if ( graph_type == LINE ) {
        ISetLineWidth ( gc, line_width );
        IDrawLine ( im_out, gc, lastx, lasty, x, y );
        ISetLineWidth ( gc, 1 );
      }
      else
        IFillRectangle ( im_out, gc, x - ( DATA_WIDTH / 2 ) + 1, y,
          DATA_WIDTH - 2, ( TOP_PAD + DATA_HEIGHT ) - y );
      /* draw a tic mark */
      ISetForeground ( gc, blue );
      IDrawLine ( im_out, gc, x, TOP_PAD + DATA_HEIGHT, x, 
        TOP_PAD + DATA_HEIGHT + 3 );
      /* label the x axis */
      sprintf ( temp, "%s", months[loop] );
      ISetForeground ( gc, black );
      if ( loop < 9 )
        IDrawString ( im_out, gc, x - 9, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      else
        IDrawString ( im_out, gc, x - 12, TOP_PAD + DATA_HEIGHT + 11,
          temp, strlen ( temp ) );
      /* save x and y for the next point */
      lastx = x;
      lasty = y;
    }
  }

  if ( display_header ) {
    sprintf ( temp, "Total retrievals for interval: %d", total );
    ISetForeground ( gc, black );
    IDrawString ( im_out, gc, 5, height - 13, temp, strlen ( temp ) );

    if ( strlen ( start_time ) && ! strlen ( stop_time ) )
      sprintf ( temp, "Time range: after %s", pretty_start );
    else if ( strlen ( stop_time ) && ! strlen ( start_time ) )
      printf ( temp, "Time range: prior to %s", pretty_stop );
    else if ( strlen ( start_time ) && strlen ( stop_time ) )
      sprintf ( temp, "Time range: %s through %s",
      pretty_start, pretty_stop );
    else
      sprintf ( temp, "Time range: all" );
    IDrawString ( im_out, gc, 5, height - 23, temp, strlen ( temp ) );
  }

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


/****************************************************************************
*
* Check to see if the entry is within our time window
*
****************************************************************************/
static int is_in_time_range ( entry )
char *entry;
{
  char *ptr;
  int is_after_start, is_before_stop;
  int loop;
  char entry_time[30];

  ptr = strstr ( entry, "[" );
  if ( ! ptr )
    return ( FALSE );
  
  ptr++;

  /* Get year */
  strncpy ( entry_time, ptr + 9, 2 );

  /* Get month */
  for ( loop = 0; loop < 12; loop++ ) {
    if ( strncmp ( months[loop], ptr + 3, 3 ) == 0 ) {
      sprintf ( entry_time + 2, "%02d", loop + 1 );
      break;
    }
  }

  /* Get day */
  strncpy ( entry_time + 4, ptr, 2 );

  /* Get hour */
  strncpy ( entry_time + 6, ptr + 12, 2 );

  /* Get minute */
  strncpy ( entry_time + 8, ptr + 15, 2 );

  /* Get second */
  strncpy ( entry_time + 10, ptr + 18, 2 );

  /* terminate string */
  entry_time[12] = '\0';
  
  if ( ! strlen ( start_time ) ) {
    is_after_start = TRUE;
  }
  else {
    is_after_start = ( strcmp ( entry_time, start_time ) > 0 );
  }
  
  if ( ! strlen ( stop_time ) ) {
    is_before_stop = TRUE;
  }
  else {
    is_before_stop = ( strcmp ( stop_time, entry_time ) > 0 );
  }

  if ( is_after_start && is_before_stop ) {
    return ( TRUE );
  }
  else
    return ( FALSE );
  
}



