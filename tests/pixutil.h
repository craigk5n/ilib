/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * White-box pixel access for tests. Ilib exposes no public per-pixel getter,
 * so tests reach into the internal IImageP layout (src/IlibP.h) to read back
 * what drawing/decoding produced.
 */
#ifndef ILIB_TEST_PIXUTIL_H
#define ILIB_TEST_PIXUTIL_H

#include <Ilib.h>
#include "IlibP.h"

static int px_width ( IImage im )  { return ( (IImageP *) im )->width; }
static int px_height ( IImage im ) { return ( (IImageP *) im )->height; }
static int px_grey ( IImage im )   { return ( (IImageP *) im )->greyscale; }

/* Red/green/blue channel of pixel (x,y). For greyscale images all three
   return the single stored value. */
static int px_r ( IImage im, int x, int y )
{
  IImageP *p = (IImageP *) im;
  if ( p->greyscale )
    return p->data[ y * p->width + x ];
  return p->data[ ( y * p->width + x ) * 3 ];
}

static int px_g ( IImage im, int x, int y )
{
  IImageP *p = (IImageP *) im;
  if ( p->greyscale )
    return p->data[ y * p->width + x ];
  return p->data[ ( y * p->width + x ) * 3 + 1 ];
}

static int px_b ( IImage im, int x, int y )
{
  IImageP *p = (IImageP *) im;
  if ( p->greyscale )
    return p->data[ y * p->width + x ];
  return p->data[ ( y * p->width + x ) * 3 + 2 ];
}

#endif /* ILIB_TEST_PIXUTIL_H */
