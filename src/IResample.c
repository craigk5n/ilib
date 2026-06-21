/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IResample.c
 *
 * Image library
 *
 * Description:
 *	Resampling transforms that need interpolation: IResize (bilinear scale
 *	to arbitrary dimensions) and IRotateAngle (rotation by an arbitrary
 *	angle). Both build a new pixel buffer and swap it in; on allocation
 *	failure the image is left unchanged and IInvalidImage is returned.
 *
 *	Sampling is bilinear (4-tap) with clamp-to-edge addressing. All channels
 *	-- including alpha -- are interpolated. IResize is a clear quality
 *	improvement over the nearest-neighbour ICopyImageScaled; note that very
 *	large downscales are smoothed but not area-averaged.
 *
 ****************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "Ilib.h"
#include "IlibP.h"

static IError _IValidImage ( IImageP *image )
{
  if ( !image )
    return ( IInvalidImage );
  if ( image->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );
  return ( INoError );
}

/* Bilinear sample the source at (fx, fy) into out[0..bpp-1], clamp-to-edge. */
static void _ISampleBilinear ( const unsigned char *src, int w, int h, int bpp,
  double fx, double fy, unsigned char *out )
{
  int x0 = (int) floor ( fx ), y0 = (int) floor ( fy );
  int x1 = x0 + 1, y1 = y0 + 1;
  double tx = fx - x0, ty = fy - y0;
  const unsigned char *p00, *p01, *p10, *p11;
  int c;

  if ( x0 < 0 )
    x0 = 0;
  else if ( x0 > w - 1 )
    x0 = w - 1;
  if ( x1 < 0 )
    x1 = 0;
  else if ( x1 > w - 1 )
    x1 = w - 1;
  if ( y0 < 0 )
    y0 = 0;
  else if ( y0 > h - 1 )
    y0 = h - 1;
  if ( y1 < 0 )
    y1 = 0;
  else if ( y1 > h - 1 )
    y1 = h - 1;

  p00 = src + ( (size_t) y0 * w + x0 ) * bpp;
  p01 = src + ( (size_t) y0 * w + x1 ) * bpp;
  p10 = src + ( (size_t) y1 * w + x0 ) * bpp;
  p11 = src + ( (size_t) y1 * w + x1 ) * bpp;

  for ( c = 0; c < bpp; c++ ) {
    double top = p00[c] * ( 1.0 - tx ) + p01[c] * tx;
    double bot = p10[c] * ( 1.0 - tx ) + p11[c] * tx;
    int v = (int) ( top * ( 1.0 - ty ) + bot * ty + 0.5 );
    out[c] = (unsigned char) ( v < 0 ? 0 : ( v > 255 ? 255 : v ) );
  }
}

IError IResize ( IImage image, unsigned int width, unsigned int height )
{
  IImageP *imagep = (IImageP *) image;
  int w, h, bpp;
  unsigned int dx, dy;
  double sx, sy;
  unsigned char *nd;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );
  if ( width == 0 || height == 0 )
    return ( IInvalidArgument );

  w = imagep->width;
  h = imagep->height;
  bpp = (int) imagep->channels;

  nd = (unsigned char *) malloc ( (size_t) width * height * bpp );
  if ( !nd )
    return ( IInvalidImage );

  /* Map destination pixel centres to source pixel centres. */
  sx = (double) w / width;
  sy = (double) h / height;
  for ( dy = 0; dy < height; dy++ ) {
    double fy = ( dy + 0.5 ) * sy - 0.5;
    for ( dx = 0; dx < width; dx++ ) {
      double fx = ( dx + 0.5 ) * sx - 0.5;
      unsigned char *out = nd + ( (size_t) dy * width + dx ) * bpp;
      _ISampleBilinear ( imagep->data, w, h, bpp, fx, fy, out );
    }
  }

  free ( imagep->data );
  imagep->data = nd;
  imagep->width = (int) width;
  imagep->height = (int) height;
  return ( INoError );
}

IError IRotateAngle ( IImage image, double degrees, IColor background )
{
  IImageP *imagep = (IImageP *) image;
  IColorP *bg;
  int w, h, bpp, nw, nh, dx, dy;
  double rad, cs, sn, halfW, halfH, halfNW, halfNH;
  unsigned char bgpx[4];
  unsigned char *nd;
  IError err = _IValidImage ( imagep );

  if ( err != INoError )
    return ( err );

  bg = _IGetColor ( (int) background );
  if ( !bg )
    return ( IInvalidColor );

  w = imagep->width;
  h = imagep->height;
  bpp = (int) imagep->channels;

  rad = degrees * PI / 180.0;
  cs = cos ( rad );
  sn = sin ( rad );

  /* Bounding box of the rotated image. */
  nw = (int) ceil ( fabs ( w * cs ) + fabs ( h * sn ) );
  nh = (int) ceil ( fabs ( w * sn ) + fabs ( h * cs ) );
  if ( nw < 1 )
    nw = 1;
  if ( nh < 1 )
    nh = 1;

  nd = (unsigned char *) malloc ( (size_t) nw * nh * bpp );
  if ( !nd )
    return ( IInvalidImage );

  bgpx[0] = bg->red;
  bgpx[1] = bg->green;
  bgpx[2] = bg->blue;
  bgpx[3] = bg->alpha;

  halfW = w / 2.0;
  halfH = h / 2.0;
  halfNW = nw / 2.0;
  halfNH = nh / 2.0;

  for ( dy = 0; dy < nh; dy++ ) {
    for ( dx = 0; dx < nw; dx++ ) {
      /* Inverse map: rotate the destination point back to source space.
         These signs make positive degrees a clockwise rotation, matching
         IRotate(). */
      double X = dx + 0.5 - halfNW;
      double Y = dy + 0.5 - halfNH;
      double sxc = X * cs + Y * sn;
      double syc = -X * sn + Y * cs;
      double fx = sxc + halfW - 0.5;
      double fy = syc + halfH - 0.5;
      unsigned char *out = nd + ( (size_t) dy * nw + dx ) * bpp;

      if ( fx < 0.0 || fy < 0.0 || fx > w - 1 || fy > h - 1 ) {
        int c;
        for ( c = 0; c < bpp; c++ )
          out[c] = bgpx[c];
      }
      else {
        _ISampleBilinear ( imagep->data, w, h, bpp, fx, fy, out );
      }
    }
  }

  free ( imagep->data );
  imagep->data = nd;
  imagep->width = nw;
  imagep->height = nh;
  return ( INoError );
}
