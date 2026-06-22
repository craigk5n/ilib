/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IGradient.c
 *
 * Image library
 *
 * Description:
 *	Linear and radial gradient fills over a rectangular region. Colors are
 *	interpolated in RGB between the two endpoint colors; pixels are written
 *	with ISetPixel so greyscale images and per-channel clamping are handled.
 *
 ****************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "Ilib.h"
#include "IlibP.h"

#define PI 3.14159265358979323846

static IError grad_setup ( IImage image, int *x, int *y, unsigned int *width,
  unsigned int *height, IColor c1, IColor c2, IColorP **p1, IColorP **p2 )
{
  IImageP *imagep = (IImageP *) image;
  if ( !imagep || imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  *p1 = _IGetColor ( (int) c1 );
  *p2 = _IGetColor ( (int) c2 );
  if ( !*p1 || !*p2 )
    return ( IInvalidColor );

  /* Clip the region to the image. */
  if ( *x < 0 ) {
    if ( (int) *width <= -*x )
      return ( IInvalidArgument );
    *width -= (unsigned int) ( -*x );
    *x = 0;
  }
  if ( *y < 0 ) {
    if ( (int) *height <= -*y )
      return ( IInvalidArgument );
    *height -= (unsigned int) ( -*y );
    *y = 0;
  }
  if ( *x >= imagep->width || *y >= imagep->height )
    return ( IInvalidArgument );
  if ( *x + (int) *width > imagep->width )
    *width = (unsigned int) ( imagep->width - *x );
  if ( *y + (int) *height > imagep->height )
    *height = (unsigned int) ( imagep->height - *y );
  if ( *width == 0 || *height == 0 )
    return ( IInvalidArgument );
  return ( INoError );
}

static void grad_set ( IImage image, int px, int py, IColorP *a, IColorP *b,
  double t )
{
  unsigned int r, g, bl;
  if ( t < 0.0 )
    t = 0.0;
  else if ( t > 1.0 )
    t = 1.0;
  r = (unsigned int) ( a->red + ( (double) b->red - a->red ) * t + 0.5 );
  g = (unsigned int) ( a->green + ( (double) b->green - a->green ) * t + 0.5 );
  bl = (unsigned int) ( a->blue + ( (double) b->blue - a->blue ) * t + 0.5 );
  ISetPixel ( image, px, py, r, g, bl );
}

IError IFillLinearGradient ( IImage image, int x, int y, unsigned int width,
  unsigned int height, IColor c1, IColor c2, double angle )
{
  IColorP *p1, *p2;
  double dx, dy, tmin, tmax, denom;
  int px, py;
  IError err = grad_setup ( image, &x, &y, &width, &height, c1, c2, &p1, &p2 );
  if ( err != INoError )
    return ( err );

  /* Gradient axis direction (angle 0 = left->right, 90 = top->bottom). */
  dx = cos ( angle * PI / 180.0 );
  dy = sin ( angle * PI / 180.0 );

  /* Project the four corners onto the axis to get the value range. */
  {
    double c[4];
    int i;
    c[0] = 0.0;
    c[1] = ( (double) width - 1 ) * dx;
    c[2] = ( (double) height - 1 ) * dy;
    c[3] = c[1] + c[2];
    tmin = tmax = c[0];
    for ( i = 1; i < 4; i++ ) {
      if ( c[i] < tmin )
        tmin = c[i];
      if ( c[i] > tmax )
        tmax = c[i];
    }
  }
  denom = tmax - tmin;
  if ( denom == 0.0 )
    denom = 1.0;

  for ( py = y; py < y + (int) height; py++ ) {
    for ( px = x; px < x + (int) width; px++ ) {
      double proj = ( px - x ) * dx + ( py - y ) * dy;
      grad_set ( image, px, py, p1, p2, ( proj - tmin ) / denom );
    }
  }
  return ( INoError );
}

IError IFillRadialGradient ( IImage image, int x, int y, unsigned int width,
  unsigned int height, int cx, int cy, unsigned int radius, IColor c1,
  IColor c2 )
{
  IColorP *p1, *p2;
  int px, py;
  double r = ( radius == 0 ) ? 1.0 : (double) radius;
  IError err = grad_setup ( image, &x, &y, &width, &height, c1, c2, &p1, &p2 );
  if ( err != INoError )
    return ( err );

  for ( py = y; py < y + (int) height; py++ ) {
    for ( px = x; px < x + (int) width; px++ ) {
      double ddx = px - cx, ddy = py - cy;
      double dist = sqrt ( ddx * ddx + ddy * ddy );
      grad_set ( image, px, py, p1, p2, dist / r );
    }
  }
  return ( INoError );
}
