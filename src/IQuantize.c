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

/* Reduce src (npixels of RGB) to at most max_colors colors, writing the result
   to dst (which may alias src). When src already has few enough colors it is
   copied through unchanged. Each output pixel depends only on the matching
   input pixel, so in-place operation is safe. */
void _IReduceColorsRGB ( const unsigned char *src, int npixels, int max_colors,
  unsigned char *dst )
{
  int *count = NULL;
  double *sumr = NULL, *sumg = NULL, *sumb = NULL;
  int *lut = NULL;
  QBucket *pop = NULL;
  int i, npop, ncolors;
  unsigned char palette[256][3];

  if ( npixels <= 0 )
    return;
  if ( max_colors < 1 )
    max_colors = 1;
  if ( max_colors > 256 )
    max_colors = 256;

  /* Always materialize the result in dst first, then quantize in place. This
     guarantees dst is valid even if a later allocation fails (we just leave
     the unquantized copy). From here on we read and write dst only. */
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

  for ( i = 0; i < npixels; i++ ) {
    int cell = QPACK ( dst[i * 3], dst[i * 3 + 1], dst[i * 3 + 2] );
    int idx = lut[cell];
    dst[i * 3] = palette[idx][0];
    dst[i * 3 + 1] = palette[idx][1];
    dst[i * 3 + 2] = palette[idx][2];
  }

done:
  free ( count );
  free ( sumr );
  free ( sumg );
  free ( sumb );
  free ( lut );
  free ( pop );
}

IError IReduceColors ( IImage image, unsigned int max_colors )
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

  _IReduceColorsRGB ( imagep->data, imagep->width * imagep->height,
    (int) max_colors, imagep->data );
  return ( INoError );
}
