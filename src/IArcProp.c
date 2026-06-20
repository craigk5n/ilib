/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IArcProp.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	28-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "Ilib.h"
#include "IlibP.h"


#define deg2rad( a ) ( ( 2.0 * PI / 360.0 ) * ( a ) )


IError IArcProperties ( IGC gc, int x, int y, int r1, int r2,
  double a1, double a2,
  int *a1_x, int *a1_y, int *a2_x, int *a2_y, int *middle_x, int *middle_y )
{
  IGCP *gcp = (IGCP *) gc;
  double a;
  int x1, y1, x2, y2, mx, my;

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );

  /* because our y is upside down, make all angles their negative */
  a1 = 360 - a1;
  a2 = 360 - a2;

  x1 = x + (int) ( r1 * cos ( deg2rad ( a1 ) ) );
  if ( a1_x )
    *a1_x = x1;
  y1 = y + (int) ( r2 * sin ( deg2rad ( a1 ) ) );
  if ( a1_y )
    *a1_y = y1;

  x2 = x + (int) ( r1 * cos ( deg2rad ( a2 ) ) );
  if ( a2_x )
    *a2_x = x2;
  y2 = y + (int) ( r2 * sin ( deg2rad ( a2 ) ) );
  if ( a2_y )
    *a2_y = y2;

  a = ( a1 + a2 ) / 2;
  mx = x + (int) ( r1 * cos ( deg2rad ( a ) ) );
  my = y + (int) ( r2 * sin ( deg2rad ( a ) ) );
  if ( middle_x )
    *middle_x = mx;
  if ( middle_y )
    *middle_y = my;

  return ( INoError );
}
