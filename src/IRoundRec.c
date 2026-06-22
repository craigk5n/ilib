/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IRoundRec.c
 *
 * Image library
 *
 * Description:
 *	Rounded rectangles, built from the existing line/arc primitives so they
 *	pick up the GC's color, line width and anti-aliasing. The corner radius is
 *	clamped to half the smaller side; a radius of 0 degenerates to a plain
 *	rectangle.
 *
 ****************************************************************************/

#include "Ilib.h"

/* Clamp the corner radius to a value that fits the rectangle. */
static int rr_clamp ( int w, int h, int r )
{
  if ( r < 0 )
    r = 0;
  if ( r * 2 > w )
    r = w / 2;
  if ( r * 2 > h )
    r = h / 2;
  return ( r );
}

IError IDrawRoundRectangle ( IImage image, IGC gc, int x, int y,
  unsigned int width, unsigned int height, unsigned int radius )
{
  int w = (int) width, h = (int) height, r;
  IError ret;

  if ( w <= 0 || h <= 0 )
    return ( IInvalidArgument );
  r = rr_clamp ( w, h, (int) radius );
  if ( r <= 0 )
    return ( IDrawRectangle ( image, gc, x, y, width, height ) );

  /* Straight edges between the corners. */
  if ( ( ret = IDrawLine ( image, gc, x + r, y, x + w - 1 - r, y ) ) )
    return ( ret );
  if ( ( ret = IDrawLine ( image, gc, x + r, y + h - 1, x + w - 1 - r,
           y + h - 1 ) ) )
    return ( ret );
  if ( ( ret = IDrawLine ( image, gc, x, y + r, x, y + h - 1 - r ) ) )
    return ( ret );
  if ( ( ret = IDrawLine ( image, gc, x + w - 1, y + r, x + w - 1,
           y + h - 1 - r ) ) )
    return ( ret );

  /* Quarter-circle corners (0 deg = right, 90 = up, 180 = left, 270 = down). */
  IDrawArc ( image, gc, x + r, y + r, r, r, 90, 180 );
  IDrawArc ( image, gc, x + w - 1 - r, y + r, r, r, 0, 90 );
  IDrawArc ( image, gc, x + w - 1 - r, y + h - 1 - r, r, r, 270, 360 );
  IDrawArc ( image, gc, x + r, y + h - 1 - r, r, r, 180, 270 );
  return ( INoError );
}

IError IFillRoundRectangle ( IImage image, IGC gc, int x, int y,
  unsigned int width, unsigned int height, unsigned int radius )
{
  int w = (int) width, h = (int) height, r;
  IError ret;

  if ( w <= 0 || h <= 0 )
    return ( IInvalidArgument );
  r = rr_clamp ( w, h, (int) radius );
  if ( r <= 0 )
    return ( IFillRectangle ( image, gc, x, y, width, height ) );

  /* A vertical band and a horizontal band cover everything but the corners. */
  if ( ( ret = IFillRectangle ( image, gc, x + r, y, (unsigned int) ( w - 2 * r ),
           (unsigned int) h ) ) )
    return ( ret );
  if ( ( ret = IFillRectangle ( image, gc, x, y + r, (unsigned int) w,
           (unsigned int) ( h - 2 * r ) ) ) )
    return ( ret );

  /* Filled quarter disks round off the four corners. */
  IFillArc ( image, gc, x + r, y + r, r, r, 90, 180 );
  IFillArc ( image, gc, x + w - 1 - r, y + r, r, r, 0, 90 );
  IFillArc ( image, gc, x + w - 1 - r, y + h - 1 - r, r, r, 270, 360 );
  IFillArc ( image, gc, x + r, y + h - 1 - r, r, r, 180, 270 );
  return ( INoError );
}
