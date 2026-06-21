/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * IQuantize.c
 *
 * Image library
 *
 * Description:
 *	Color reduction (quantization) via median cut. Used to fit an image
 *	into a limited palette, most notably for 8-bit GIF output.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Ilib.h"
#include "IlibP.h"

/* The histogram works at 5 bits/channel (32768 cells). That precision only
   matters once we are *already* reducing a >max_colors image (a lossy
   operation); images with few enough colors are passed through exactly. */
#define QBITS 5
#define QSHIFT ( 8 - QBITS )
#define QLEVELS ( 1 << QBITS )        /* 32 */
#define QCELLS ( 1 << ( QBITS * 3 ) ) /* 32768 */
#define QPACK( r, g, b )                       \
  ( ( ( ( r ) >> QSHIFT ) << ( QBITS * 2 ) ) | \
    ( ( ( g ) >> QSHIFT ) << QBITS ) | ( ( b ) >> QSHIFT ) )

typedef struct {
  unsigned char r5, g5, b5; /* 5-bit channel values */
  int count;                /* pixels in this cell */
  double sr, sg, sb;        /* sums of original 8-bit values (for averaging) */
} QBucket;

typedef struct {
  int lo, hi; /* [lo, hi) slice of the bucket array */
} QBox;

/* Comparison axis for qsort (0=r, 1=g, 2=b). Not thread-safe, but neither is
   the rest of the library's global color state. */
static int q_axis = 0;

static int q_cmp ( const void *a, const void *b )
{
  const QBucket *ba = (const QBucket *) a;
  const QBucket *bb = (const QBucket *) b;
  int va = q_axis == 0 ? ba->r5 : ( q_axis == 1 ? ba->g5 : ba->b5 );
  int vb = q_axis == 0 ? bb->r5 : ( q_axis == 1 ? bb->g5 : bb->b5 );
  return va - vb;
}

/* Longest axis (and its extent) of the buckets in box. */
static int q_box_extent ( const QBucket *pop, const QBox *box, int *axis_ret )
{
  int rmin = QLEVELS, rmax = -1, gmin = QLEVELS, gmax = -1, bmin = QLEVELS,
      bmax = -1, i, rext, gext, bext;
  for ( i = box->lo; i < box->hi; i++ ) {
    if ( pop[i].r5 < rmin )
      rmin = pop[i].r5;
    if ( pop[i].r5 > rmax )
      rmax = pop[i].r5;
    if ( pop[i].g5 < gmin )
      gmin = pop[i].g5;
    if ( pop[i].g5 > gmax )
      gmax = pop[i].g5;
    if ( pop[i].b5 < bmin )
      bmin = pop[i].b5;
    if ( pop[i].b5 > bmax )
      bmax = pop[i].b5;
  }
  rext = rmax - rmin;
  gext = gmax - gmin;
  bext = bmax - bmin;
  if ( rext >= gext && rext >= bext ) {
    *axis_ret = 0;
    return rext;
  }
  if ( gext >= bext ) {
    *axis_ret = 1;
    return gext;
  }
  *axis_ret = 2;
  return bext;
}

/* Median-cut the populated buckets into <= max_colors boxes; write each box's
   average color into palette[] and return the number of boxes. lut[] is filled
   so lut[QPACK(r,g,b)] is the palette index for any cell in a box. */
static int q_median_cut ( QBucket *pop, int npop, int max_colors,
  unsigned char palette[][3], int *lut )
{
  QBox *boxes = malloc ( (size_t) max_colors * sizeof ( QBox ) );
  int nboxes = 1, i, j;

  if ( !boxes )
    return 0;
  boxes[0].lo = 0;
  boxes[0].hi = npop;

  while ( nboxes < max_colors ) {
    int best = -1, best_ext = 0, axis = 0, a;
    long total, acc;
    int split;
    /* Pick the splittable box with the largest extent. */
    for ( i = 0; i < nboxes; i++ ) {
      if ( boxes[i].hi - boxes[i].lo < 2 )
        continue;
      a = 0;
      {
        int ext = q_box_extent ( pop, &boxes[i], &a );
        if ( ext > best_ext ) {
          best_ext = ext;
          best = i;
          axis = a;
        }
      }
    }
    if ( best < 0 )
      break; /* nothing left to split */

    q_axis = axis;
    qsort ( pop + boxes[best].lo, (size_t) ( boxes[best].hi - boxes[best].lo ),
      sizeof ( QBucket ), q_cmp );

    /* Split at the median by pixel count. */
    total = 0;
    for ( i = boxes[best].lo; i < boxes[best].hi; i++ )
      total += pop[i].count;
    acc = 0;
    split = boxes[best].lo + 1;
    for ( i = boxes[best].lo; i < boxes[best].hi - 1; i++ ) {
      acc += pop[i].count;
      if ( acc * 2 >= total ) {
        split = i + 1;
        break;
      }
    }

    boxes[nboxes].lo = split;
    boxes[nboxes].hi = boxes[best].hi;
    boxes[best].hi = split;
    nboxes++;
  }

  for ( i = 0; i < nboxes; i++ ) {
    double sr = 0, sg = 0, sb = 0;
    long cnt = 0;
    for ( j = boxes[i].lo; j < boxes[i].hi; j++ ) {
      sr += pop[j].sr;
      sg += pop[j].sg;
      sb += pop[j].sb;
      cnt += pop[j].count;
    }
    if ( cnt == 0 )
      cnt = 1;
    palette[i][0] = (unsigned char) ( sr / cnt + 0.5 );
    palette[i][1] = (unsigned char) ( sg / cnt + 0.5 );
    palette[i][2] = (unsigned char) ( sb / cnt + 0.5 );
    for ( j = boxes[i].lo; j < boxes[i].hi; j++ )
      lut[QPACK ( pop[j].r5 << QSHIFT, pop[j].g5 << QSHIFT,
        pop[j].b5 << QSHIFT )] = i;
  }

  free ( boxes );
  return nboxes;
}

/* Return 1 if the RGB pixels contain at most max_colors distinct colors. Uses
   a small open-addressing set bounded to max_colors entries, so it bails out
   as soon as one too many colors is seen. */
static int q_within_limit ( const unsigned char *rgb, int npixels,
  int max_colors )
{
  int cap = max_colors * 2 + 1;
  int *keys = malloc ( (size_t) cap * sizeof ( int ) );
  int i, count = 0, within = 1;

  if ( !keys )
    return 0; /* treat as "needs quantize" on OOM */
  for ( i = 0; i < cap; i++ )
    keys[i] = -1;

  for ( i = 0; i < npixels; i++ ) {
    int key = ( rgb[i * 3] << 16 ) | ( rgb[i * 3 + 1] << 8 ) | rgb[i * 3 + 2];
    int h = (int) ( ( (unsigned int) key * 2654435761u ) % (unsigned int) cap );
    while ( keys[h] != -1 && keys[h] != key )
      h = ( h + 1 ) % cap;
    if ( keys[h] == -1 ) {
      if ( count >= max_colors ) {
        within = 0;
        break;
      }
      keys[h] = key;
      count++;
    }
  }
  free ( keys );
  return within;
}

/* Nearest palette entry to (r, g, b) by squared Euclidean distance. */
static int q_nearest ( const unsigned char palette[][3], int ncolors, int r,
  int g, int b )
{
  long best = -1;
  int besti = 0, i;
  for ( i = 0; i < ncolors; i++ ) {
    long dr = r - palette[i][0], dg = g - palette[i][1], db = b - palette[i][2];
    long d = dr * dr + dg * dg + db * db;
    if ( best < 0 || d < best ) {
      best = d;
      besti = i;
    }
  }
  return ( besti );
}

#define QCLAMP( v ) ( ( v ) < 0 ? 0 : ( ( v ) > 255 ? 255 : ( v ) ) )

/* Core quantizer. Reduce a width*height RGB image (src) to at most max_colors
   colors, writing to dst (which may alias src). With dither set, error is
   diffused (Floyd-Steinberg) so gradients keep their shape instead of banding.
   When src already has few enough colors it is copied through unchanged. */
static void q_quantize ( const unsigned char *src, int width, int height,
  int max_colors, int dither, unsigned char *dst )
{
  int *count = NULL;
  double *sumr = NULL, *sumg = NULL, *sumb = NULL;
  int *lut = NULL, *ecur = NULL, *enext = NULL;
  QBucket *pop = NULL;
  int i, npop, ncolors, npixels = width * height;
  unsigned char palette[256][3];

  if ( npixels <= 0 )
    return;
  if ( max_colors < 1 )
    max_colors = 1;
  if ( max_colors > 256 )
    max_colors = 256;

  /* Materialize the result in dst first, so it stays valid even if a later
     allocation fails (we just leave the unquantized copy). */
  if ( dst != src )
    memcpy ( dst, src, (size_t) npixels * 3 );

  if ( q_within_limit ( dst, npixels, max_colors ) )
    return;

  count = calloc ( QCELLS, sizeof ( int ) );
  sumr = calloc ( QCELLS, sizeof ( double ) );
  sumg = calloc ( QCELLS, sizeof ( double ) );
  sumb = calloc ( QCELLS, sizeof ( double ) );
  lut = calloc ( QCELLS, sizeof ( int ) );
  if ( !count || !sumr || !sumg || !sumb || !lut )
    goto done; /* leave dst unchanged on OOM */

  for ( i = 0; i < npixels; i++ ) {
    unsigned char r = dst[i * 3], g = dst[i * 3 + 1], b = dst[i * 3 + 2];
    int cell = QPACK ( r, g, b );
    count[cell]++;
    sumr[cell] += r;
    sumg[cell] += g;
    sumb[cell] += b;
  }

  npop = 0;
  for ( i = 0; i < QCELLS; i++ )
    if ( count[i] )
      npop++;
  pop = malloc ( (size_t) npop * sizeof ( QBucket ) );
  if ( !pop )
    goto done;
  npop = 0;
  for ( i = 0; i < QCELLS; i++ ) {
    if ( !count[i] )
      continue;
    pop[npop].r5 = ( i >> ( QBITS * 2 ) ) & ( QLEVELS - 1 );
    pop[npop].g5 = ( i >> QBITS ) & ( QLEVELS - 1 );
    pop[npop].b5 = i & ( QLEVELS - 1 );
    pop[npop].count = count[i];
    pop[npop].sr = sumr[i];
    pop[npop].sg = sumg[i];
    pop[npop].sb = sumb[i];
    npop++;
  }

  ncolors = q_median_cut ( pop, npop, max_colors, palette, lut );
  if ( ncolors < 1 )
    goto done;

  /* Upgrade lut to a complete nearest-color table over every cell (the median
     cut only filled populated cells); dithering pushes values into empties. */
  for ( i = 0; i < QCELLS; i++ ) {
    int r = ( ( ( i >> ( QBITS * 2 ) ) & ( QLEVELS - 1 ) ) << QSHIFT ) +
            ( 1 << ( QSHIFT - 1 ) );
    int g = ( ( ( i >> QBITS ) & ( QLEVELS - 1 ) ) << QSHIFT ) +
            ( 1 << ( QSHIFT - 1 ) );
    int b =
      ( ( i & ( QLEVELS - 1 ) ) << QSHIFT ) + ( 1 << ( QSHIFT - 1 ) );
    lut[i] = q_nearest ( palette, ncolors, r, g, b );
  }

  if ( !dither ) {
    for ( i = 0; i < npixels; i++ ) {
      int idx = lut[QPACK ( dst[i * 3], dst[i * 3 + 1], dst[i * 3 + 2] )];
      dst[i * 3] = palette[idx][0];
      dst[i * 3 + 1] = palette[idx][1];
      dst[i * 3 + 2] = palette[idx][2];
    }
    goto done;
  }

  /* Floyd-Steinberg error diffusion. Error is carried in two int row buffers
     (current and next) rather than in the image, so each pixel still reads its
     untouched original. */
  ecur = calloc ( (size_t) width * 3, sizeof ( int ) );
  enext = calloc ( (size_t) width * 3, sizeof ( int ) );
  if ( !ecur || !enext )
    goto done;
  {
    int x, y, c;
    for ( y = 0; y < height; y++ ) {
      memset ( enext, 0, (size_t) width * 3 * sizeof ( int ) );
      for ( x = 0; x < width; x++ ) {
        int idx = ( y * width + x ) * 3, pi;
        int oldv[3], newv[3], err[3];
        for ( c = 0; c < 3; c++ )
          oldv[c] = QCLAMP ( dst[idx + c] + ecur[x * 3 + c] );
        pi = lut[QPACK ( oldv[0], oldv[1], oldv[2] )];
        for ( c = 0; c < 3; c++ ) {
          newv[c] = palette[pi][c];
          err[c] = oldv[c] - newv[c];
          dst[idx + c] = (unsigned char) newv[c];
        }
        for ( c = 0; c < 3; c++ ) {
          if ( x + 1 < width )
            ecur[( x + 1 ) * 3 + c] += err[c] * 7 / 16;
          if ( x > 0 )
            enext[( x - 1 ) * 3 + c] += err[c] * 3 / 16;
          enext[x * 3 + c] += err[c] * 5 / 16;
          if ( x + 1 < width )
            enext[( x + 1 ) * 3 + c] += err[c] * 1 / 16;
        }
      }
      {
        int *tmp = ecur;
        ecur = enext;
        enext = tmp;
      }
    }
  }

done:
  free ( count );
  free ( sumr );
  free ( sumg );
  free ( sumb );
  free ( lut );
  free ( pop );
  free ( ecur );
  free ( enext );
}

/* Reduce src (npixels of RGB) to at most max_colors colors, in place-safe. */
void _IReduceColorsRGB ( const unsigned char *src, int npixels, int max_colors,
  unsigned char *dst )
{
  q_quantize ( src, npixels, 1, max_colors, 0, dst );
}

/* As above, but Floyd-Steinberg dithered when dither is non-zero (needs the
   real width/height for error diffusion). */
void _IReduceColorsRGBDither ( const unsigned char *src, int width, int height,
  int max_colors, int dither, unsigned char *dst )
{
  q_quantize ( src, width, height, max_colors, dither, dst );
}

static IError q_reduce_image ( IImage image, unsigned int max_colors,
  int dither )
{
  IImageP *imagep = (IImageP *) image;

  if ( !imagep )
    return ( IInvalidImage );
  if ( imagep->magic != IMAGIC_IMAGE )
    return ( IInvalidImage );

  /* Greyscale images already have at most 256 colors. */
  if ( imagep->greyscale )
    return ( INoError );
  if ( imagep->width <= 0 || imagep->height <= 0 )
    return ( INoError );

  q_quantize ( imagep->data, imagep->width, imagep->height, (int) max_colors,
    dither, imagep->data );
  return ( INoError );
}

IError IReduceColors ( IImage image, unsigned int max_colors )
{
  return ( q_reduce_image ( image, max_colors, 0 ) );
}

IError IDither ( IImage image, unsigned int max_colors )
{
  return ( q_reduce_image ( image, max_colors, 1 ) );
}
