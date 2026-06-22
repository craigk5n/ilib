/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IFlood.c
 *
 * Image library
 *
 * Description:
 *	Portable routines to manipulate raster images.
 *
 * History:
 *	25-Oct-04	Craig Knudsen	cknudsen@cknudsen.com
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


typedef struct {
  int x, y;
} IFloodSeed;

/* Push a seed onto the dynamic stack, growing it as needed. Returns 0 on OOM. */
static int flood_push ( IFloodSeed **stack, int *top, int *cap, int x, int y )
{
  if ( *top == *cap ) {
    int ncap = *cap ? *cap * 2 : 256;
    IFloodSeed *ns =
      (IFloodSeed *) realloc ( *stack, (size_t) ncap * sizeof ( IFloodSeed ) );
    if ( !ns )
      return ( 0 );
    *stack = ns;
    *cap = ncap;
  }
  ( *stack )[*top].x = x;
  ( *stack )[( *top )++].y = y;
  return ( 1 );
}

IError IFloodFill ( IImage image, IGC gc, int x, int y )
{
  IGCP *gcp = (IGCP *) gc;
  IImageP *imagep = (IImageP *) image;
  int w, h, i, top = 0, cap = 0;
  IFloodSeed *stack = NULL;
  IColorP color = { 0 };
  IColorP origColor = { 0 }; /* color we are replacing with flood fill */

  if ( !gcp )
    return ( IInvalidGC );
  if ( gcp->magic != IMAGIC_GC )
    return ( IInvalidGC );
  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  if ( !gcp->foreground )
    return ( IInvalidGC );

  w = imagep->width;
  h = imagep->height;
  if ( x < 0 || x >= w || y < 0 || y >= h )
    return ( INoError );

  _IGetPointColor ( imagep, x, y, origColor );
  /* If the fill color is the target color there is nothing to do -- and the
     run-matching below would never terminate. */
  if ( _IColorsMatch ( ( *gcp->foreground ), origColor ) )
    return ( INoError );

  /* Iterative scanline flood fill: a heap-backed seed stack instead of
     recursion, so a large uniform region cannot overflow the call stack. */
  if ( !flood_push ( &stack, &top, &cap, x, y ) )
    return ( IInvalidImage );

  while ( top > 0 ) {
    int sx = stack[--top].x, sy = stack[top].y, L, R;
    _IGetPointColor ( imagep, sx, sy, color );
    if ( !_IColorsMatch ( color, origColor ) )
      continue; /* already filled (via another seed) or not part of the region */

    L = sx;
    while ( L - 1 >= 0 ) {
      _IGetPointColor ( imagep, L - 1, sy, color );
      if ( !_IColorsMatch ( color, origColor ) )
        break;
      L--;
    }
    R = sx;
    while ( R + 1 < w ) {
      _IGetPointColor ( imagep, R + 1, sy, color );
      if ( !_IColorsMatch ( color, origColor ) )
        break;
      R++;
    }

    for ( i = L; i <= R; i++ )
      _ISetPoint ( imagep, gcp, i, sy );

    for ( i = L; i <= R; i++ ) {
      if ( sy > 0 ) {
        _IGetPointColor ( imagep, i, sy - 1, color );
        if ( _IColorsMatch ( color, origColor ) ) {
          if ( !flood_push ( &stack, &top, &cap, i, sy - 1 ) ) {
            free ( stack );
            return ( IInvalidImage );
          }
        }
      }
      if ( sy < h - 1 ) {
        _IGetPointColor ( imagep, i, sy + 1, color );
        if ( _IColorsMatch ( color, origColor ) ) {
          if ( !flood_push ( &stack, &top, &cap, i, sy + 1 ) ) {
            free ( stack );
            return ( IInvalidImage );
          }
        }
      }
    }
  }

  free ( stack );
  return ( INoError );
}
