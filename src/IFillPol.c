/*
 * IFillPol.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * Limitations:
 *	This function only handles CONVEX non-intersecting polygons.
 *	That means if you scan from left to right, you will pass through
 *	the polygon once.  This make this algorith (1) faster and
 *	(2) easier.  We also assume this is _closed_ polygon.
 *
 * History:
 *	22-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"


typedef struct {
  int x1, y1, x2, y2;
  double slope;
} linetype;


#undef max
#define max(a,b)	( a > b ) ? a : b
#undef min
#define min(a,b)	( a < b ) ? a : b

/*
** Calculate slope.  Keep in mind that our y coordinate is reverse
** compared to the standard math coordinate system.
*/
static void set_line_slope ( line )
linetype *line;
{
  if ( abs ( line->x2 - line->x1 ) < 0.01 )
    line->slope = 0;
  else
    line->slope = ( (double)line->y2 - (double)line->y1 ) /
      ( (double) line->x2 - (double) line->x1 );
}



/*
** Does a line include the specified y value?
*/
static int line_includes_y_value ( line, yval )
linetype line;
int yval;
{
  if ( line.y1 <= yval && line.y2 >= yval )
    return 1;
  if ( line.y2 <= yval && line.y1 >= yval )
    return 1;
  return 0;
}

static int get_intersection_x_value ( line, yval )
linetype line;
int yval;
{
  double b, ret;

  if ( line.x1 == line.x2 )
    ret = (double) line.x1;

  else if ( line.y1 == line.y2 )
    ret = (double) line.x1;

  else {
    /* calc b now
    ** y = mx + b
    ** b = y - mx
    */
    b = line.y2 - ( line.slope * (double) line.x2 );

    /*
    ** now determine x value
    ** x = (y - b) / m)
    */
    ret = ( ( (double)yval - b ) / line.slope );
  }

  return (int) ret;
}



IError IFillPolygon ( image, gc, points, npoints )
IImage image;
IGC gc;
IPoint *points;
int npoints;
{
  IGCP *gcp = (IGCP *)gc;
  IImageP *imagep = (IImageP *)image;
  int loop;
  int maxY, minY;
  IPoint *pts;
  int left, right, xval;
  int npts, yloop;
  linetype *lines;
  int nlines;
  int found;
  int save_line_width;

  if ( ! gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( ! imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( npoints < 2 )
    return ( INoError );

  save_line_width = gcp->line_width;
  gcp->line_width = 1;

  /* create an array of lines */
  nlines = npoints;
  lines = (linetype *) malloc ( sizeof ( linetype ) * nlines );
  for ( loop = 1; loop < npoints; loop++ ) {
    lines[loop].x1 = points[loop-1].x;
    lines[loop].y1 = points[loop-1].y;
    lines[loop].x2 = points[loop].x;
    lines[loop].y2 = points[loop].y;
    set_line_slope ( &lines[loop] );
  }
  /* last line connects first and last points */
  lines[0].x1 = points[npoints-1].x;
  lines[0].y1 = points[npoints-1].y;
  lines[0].x2 = points[0].x;
  lines[0].y2 = points[0].y;
  set_line_slope ( &lines[0] );

/* debugging code
  for ( loop = 0; loop < nlines; loop++ ) {
    printf ( "Line %d: (%d,%d) to (%d,%d) with slope = %.2f\n",
      loop, lines[loop].x1, lines[loop].y1, lines[loop].x2, lines[loop].y2,
      (float)lines[loop].slope );
  }
*/

  /* calculate the min and max y values */
  minY = maxY = points[0].y;
  for ( loop = 1; loop < npoints; loop++ ) {
    minY = points[loop].y < minY ? points[loop].y : minY;
    maxY = points[loop].y > maxY ? points[loop].y : maxY;
  }
  npts = maxY - minY + 1;
  pts = (IPoint *) malloc ( sizeof ( IPoint ) * npts );
  memset ( pts, '\0', sizeof ( IPoint ) * npts );

  /* now loop through from lowest y to top y */
  for ( yloop = minY; yloop <= maxY; yloop++ ) {
    /* now determine which lines of this polygon intersect this y value */
    found = 0;
    for ( loop = 0; loop < nlines; loop++ ) {
      if ( line_includes_y_value ( lines[loop], yloop ) ) {
        /* don't know if this is the left-most or right-most if this is
        ** first point found...
        */
        if ( ! found ) {
          /* get intersection of this line and y */
          if ( lines[loop].y1 == lines[loop].y2 ) {
            left = min ( lines[loop].x1, lines[loop].x2 );
            right = max ( lines[loop].x1, lines[loop].x2 );
          } else {
            left = right = get_intersection_x_value ( lines[loop], yloop );
          }
          found++;
        } else {
          if ( lines[loop].y1 == lines[loop].y2 ) {
            left = min ( left, lines[loop].x1 );
            left = min ( left, lines[loop].x2 );
            right = max ( right, lines[loop].x1 );
            right = max ( right, lines[loop].x2 );
          } else {
            xval = get_intersection_x_value ( lines[loop], yloop );
            left = min ( left, xval );
            right = max ( right, xval );
          }
          found++;
          /* printf ( "left = %d, right = %d\n", left, right ); */
        }
      }
    }

    if ( found >= 2 ) {
      IDrawLine ( image, gc, left, yloop, right, yloop );
    } else if ( found == 1 && left == right ) {
      IDrawLine ( image, gc, left, yloop, right, yloop );
    } else if ( found == 1 && left != right ) {
      /* Eek.  This really shouldn't happen */
      fprintf ( stderr, "Ilib bug no. 34534345\n" );
    }
  }

  free ( lines );
  free ( pts );

  gcp->line_width = save_line_width;
 
  return ( INoError );
}



