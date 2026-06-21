/* SPDX-License-Identifier: GPL-2.0-only */
/* Image format I/O: round-trips for the always-available raw formats,
   graceful handling of malformed input, and clean behaviour for the optional
   codecs whether or not they were compiled in. */

#include <stdio.h>
#include <stdlib.h>
#include <Ilib.h>
#include "pixutil.h"
#include "greatest.h"

/* Write image to a temp file in `format`, read it back, return the new image
   (or NULL). Sets *write_ret/*read_ret to the IError of each step. */
static IImage roundtrip ( IImage im, IFileFormat format,
  IError *write_ret, IError *read_ret )
{
  IImage back = NULL;
  FILE *fp = tmpfile ();
  if ( !fp ) {
    *write_ret = *read_ret = IErrorWriting;
    return NULL;
  }
  *write_ret = IWriteImageFile ( fp, im, format, IOPTION_NONE );
  rewind ( fp );
  *read_ret = IReadImageFile ( fp, format, IOPTION_NONE, &back );
  fclose ( fp );
  return back;
}

TEST ppm_roundtrip ( void )
{
  IImage im = ICreateImage ( 6, 4, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor red = IAllocColor ( 255, 0, 0 );
  IError wr, rd;
  IImage back;

  ISetForeground ( gc, red );
  IDrawPoint ( im, gc, 2, 1 );

  back = roundtrip ( im, IFORMAT_PPM, &wr, &rd );
  ASSERT_EQ ( INoError, wr );
  ASSERT_EQ ( INoError, rd );
  ASSERT ( back != NULL );
  ASSERT_EQ ( 6, px_width ( back ) );
  ASSERT_EQ ( 4, px_height ( back ) );
  ASSERT_EQ ( 255, px_r ( back, 2, 1 ) );
  ASSERT_EQ ( 0, px_g ( back, 2, 1 ) );
  ASSERT_EQ ( 0, px_b ( back, 2, 1 ) );

  IFreeImage ( back );
  IFreeColor ( red );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST pgm_roundtrip_greyscale ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_GREYSCALE );
  IGC gc = ICreateGC ();
  IColor grey = IAllocColor ( 128, 128, 128 );
  IError wr, rd;
  IImage back;

  ISetForeground ( gc, grey );
  IDrawPoint ( im, gc, 3, 3 );

  back = roundtrip ( im, IFORMAT_PGM, &wr, &rd );
  ASSERT_EQ ( INoError, wr );
  ASSERT_EQ ( INoError, rd );
  ASSERT ( back != NULL );
  ASSERT_EQ ( 5, px_width ( back ) );
  ASSERT_EQ ( 128, px_r ( back, 3, 3 ) );

  IFreeImage ( back );
  IFreeColor ( grey );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* BMP is now writable as well as readable. Use a non-square, non-4-aligned
   width so the row-padding path is exercised, and check a couple of pixels and
   the corners survive the bottom-up row ordering. */
TEST bmp_roundtrip ( void )
{
  IImage im = ICreateImage ( 5, 3, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor red = IAllocColor ( 255, 0, 0 );
  IColor green = IAllocColor ( 0, 255, 0 );
  IError wr, rd;
  IImage back;

  ISetForeground ( gc, red );
  IDrawPoint ( im, gc, 0, 0 ); /* top-left */
  ISetForeground ( gc, green );
  IDrawPoint ( im, gc, 4, 2 ); /* bottom-right */

  back = roundtrip ( im, IFORMAT_BMP, &wr, &rd );
  ASSERT_EQ ( INoError, wr );
  ASSERT_EQ ( INoError, rd );
  ASSERT ( back != NULL );
  ASSERT_EQ ( 5, px_width ( back ) );
  ASSERT_EQ ( 3, px_height ( back ) );
  ASSERT_EQ ( 255, px_r ( back, 0, 0 ) );
  ASSERT_EQ ( 0, px_g ( back, 0, 0 ) );
  ASSERT_EQ ( 255, px_g ( back, 4, 2 ) );
  ASSERT_EQ ( 0, px_r ( back, 4, 2 ) );

  IFreeImage ( back );
  IFreeColor ( red );
  IFreeColor ( green );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* Feeding garbage to a decoder must return an error, not crash. */
TEST garbage_ppm_rejected ( void )
{
  FILE *fp = tmpfile ();
  IImage back = NULL;
  IError rd;
  fputs ( "this is definitely not a PPM image file\n", fp );
  rewind ( fp );
  rd = IReadImageFile ( fp, IFORMAT_PPM, IOPTION_NONE, &back );
  fclose ( fp );
  ASSERT ( rd != INoError );
  if ( back )
    IFreeImage ( back );
  PASS ();
}

/* A truncated header must also be rejected cleanly. */
TEST truncated_ppm_rejected ( void )
{
  FILE *fp = tmpfile ();
  IImage back = NULL;
  IError rd;
  fputs ( "P6\n", fp );
  rewind ( fp );
  rd = IReadImageFile ( fp, IFORMAT_PPM, IOPTION_NONE, &back );
  fclose ( fp );
  ASSERT ( rd != INoError );
  if ( back )
    IFreeImage ( back );
  PASS ();
}

/* Count distinct RGB colors in an image, returning at most cap+1 (cap+1 means
   "more than cap"). Linear scan -- fine for the small test images here. */
static int count_colors ( IImage im, int cap )
{
  IImageP *p = (IImageP *) im;
  int n = p->width * p->height, i, j, ndistinct = 0;
  int *seen = malloc ( (size_t) ( cap + 1 ) * sizeof ( int ) );
  if ( !seen )
    return -1;
  for ( i = 0; i < n; i++ ) {
    int key = ( p->data[i * 3] << 16 ) | ( p->data[i * 3 + 1] << 8 ) |
              p->data[i * 3 + 2];
    int found = 0;
    for ( j = 0; j < ndistinct; j++ ) {
      if ( seen[j] == key ) {
        found = 1;
        break;
      }
    }
    if ( !found ) {
      if ( ndistinct > cap ) {
        ndistinct = cap + 1;
        break;
      }
      seen[ndistinct++] = key;
    }
  }
  free ( seen );
  return ndistinct;
}

/* A w*h RGB image whose pixels are all distinct: pixel i gets color
   (i & 0xff, (i >> 8) & 0xff, 0), so an image with > 256 pixels has > 256
   colors. */
static IImage make_rainbow ( int w, int h )
{
  IImage im = ICreateImage ( w, h, IOPTION_NONE );
  IImageP *p = (IImageP *) im;
  int i, n = w * h;
  for ( i = 0; i < n; i++ ) {
    p->data[i * 3] = i & 0xff;
    p->data[i * 3 + 1] = ( i >> 8 ) & 0xff;
    p->data[i * 3 + 2] = 0;
  }
  return im;
}

/* IReduceColors must bring a >256-color image down to a GIF-sized palette
   while leaving the dimensions intact. */
TEST reduce_colors_caps_palette ( void )
{
  IImage im = make_rainbow ( 24, 24 ); /* 576 distinct colors */
  ASSERT ( count_colors ( im, 256 ) > 256 );
  ASSERT_EQ ( INoError, IReduceColors ( im, 256 ) );
  ASSERT ( count_colors ( im, 256 ) <= 256 );
  ASSERT_EQ ( 24, px_width ( im ) );
  ASSERT_EQ ( 24, px_height ( im ) );
  IFreeImage ( im );
  PASS ();
}

/* When an image already fits, reduction is lossless: exact colors survive. */
TEST reduce_colors_preserves_few ( void )
{
  IImage im = ICreateImage ( 4, 1, IOPTION_NONE );
  IImageP *p = (IImageP *) im;
  /* four deliberately close-but-distinct colors */
  unsigned char want[4][3] = {
    { 10, 20, 30 }, { 11, 20, 30 }, { 200, 100, 50 }, { 201, 100, 50 } };
  int i;
  for ( i = 0; i < 4; i++ ) {
    p->data[i * 3] = want[i][0];
    p->data[i * 3 + 1] = want[i][1];
    p->data[i * 3 + 2] = want[i][2];
  }
  ASSERT_EQ ( INoError, IReduceColors ( im, 256 ) );
  for ( i = 0; i < 4; i++ ) {
    ASSERT_EQ ( want[i][0], px_r ( im, i, 0 ) );
    ASSERT_EQ ( want[i][1], px_g ( im, i, 0 ) );
    ASSERT_EQ ( want[i][2], px_b ( im, i, 0 ) );
  }
  IFreeImage ( im );
  PASS ();
}

/* A NULL/bogus handle is rejected. */
TEST reduce_colors_rejects_bad_handle ( void )
{
  unsigned int not_an_image = 0;
  ASSERT_EQ ( IInvalidImage, IReduceColors ( NULL, 256 ) );
  ASSERT_EQ ( IInvalidImage, IReduceColors ( (IImage) &not_an_image, 256 ) );
  PASS ();
}

/* Count adjacent-pixel color changes along the middle row. */
static int row_transitions ( IImage im, int w, int y )
{
  int x, t = 0;
  unsigned int pr, pg, pb, r, g, b;
  IGetPixel ( im, 0, y, &pr, &pg, &pb );
  for ( x = 1; x < w; x++ ) {
    IGetPixel ( im, x, y, &r, &g, &b );
    if ( r != pr || g != pg || b != pb )
      t++;
    pr = r;
    pg = g;
    pb = b;
  }
  return ( t );
}

/* Dithering a smooth gradient down to 2 colors must scatter both palette colors
   across the transition (many changes), where a plain reduce yields ~one hard
   edge. */
TEST dither_diffuses_gradient ( void )
{
  int w = 64, h = 8, x, y;
  IImage plain = ICreateImage ( w, h, IOPTION_NONE );
  IImage dith = ICreateImage ( w, h, IOPTION_NONE );

  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      unsigned int v = (unsigned int) ( x * 255 / ( w - 1 ) );
      ISetPixel ( plain, x, y, v, v, v );
      ISetPixel ( dith, x, y, v, v, v );
    }
  }
  ASSERT_EQ ( INoError, IReduceColors ( plain, 2 ) );
  ASSERT_EQ ( INoError, IDither ( dith, 2 ) );

  /* Plain reduction: a single hard edge. Dithered: many interleaved changes. */
  ASSERT ( row_transitions ( dith, w, h / 2 ) >
           row_transitions ( plain, w, h / 2 ) + 3 );

  IFreeImage ( plain );
  IFreeImage ( dith );
  PASS ();
}

TEST dither_rejects_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidImage, IDither ( NULL, 16 ) );
  PASS ();
}

/* The GIF writer auto-quantizes a >256-color image down to a 256-color
   palette: writing one must succeed cleanly (when giflib is compiled in) or
   fail cleanly otherwise -- never crash or report a palette overflow. Palette
   correctness itself is covered by reduce_colors_caps_palette; we don't read
   the GIF back here because giflib closes the file descriptor on write, which
   makes reusing the same stream unreliable. */
TEST gif_write_quantizes ( void )
{
  IImage im = make_rainbow ( 24, 24 ); /* 576 colors -> must be quantized */
  FILE *fp = tmpfile ();
  IError wr;
  ASSERT ( fp != NULL );
  wr = IWriteImageFile ( fp, im, IFORMAT_GIF, IOPTION_NONE );
  ASSERT ( wr == INoError || wr == IFunctionNotImplemented );
  fclose ( fp );
  IFreeImage ( im );
  PASS ();
}

/* Writing an optional codec must either succeed (when compiled in) or fail
   cleanly (when not) -- never crash. When it round-trips, the image must come
   back intact. */
TEST optional_codec_write_is_clean ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  IError wr, rd;
  IImage back = roundtrip ( im, IFORMAT_PNG, &wr, &rd );
  if ( wr == INoError ) {
    ASSERT_EQ ( INoError, rd );
    ASSERT ( back != NULL );
    ASSERT_EQ ( 4, px_width ( back ) );
    IFreeImage ( back );
  }
  else {
    /* not compiled in: a clean error, and no image produced */
    ASSERT ( back == NULL );
  }
  IFreeImage ( im );
  PASS ();
}

/* WebP round-trip (when libwebp is compiled in). WebP is lossy, so a solid
   colour is checked with tolerance; alpha is preserved (encoded losslessly). */
TEST webp_roundtrip ( void )
{
  IImage im = ICreateImage ( 16, 16, IOPTION_NONE );
  IError wr, rd;
  IImage back;
  int x, y;

  for ( y = 0; y < 16; y++ )
    for ( x = 0; x < 16; x++ )
      ISetPixel ( im, x, y, 200, 100, 50 );

  back = roundtrip ( im, IFORMAT_WEBP, &wr, &rd );
  if ( wr == INoError ) {
    unsigned int r, g, b;
    ASSERT_EQ ( INoError, rd );
    ASSERT ( back != NULL );
    ASSERT_EQ ( 16, px_width ( back ) );
    ASSERT_EQ ( 16, px_height ( back ) );
    IGetPixel ( back, 8, 8, &r, &g, &b );
    ASSERT ( abs ( (int) r - 200 ) <= 12 );
    ASSERT ( abs ( (int) g - 100 ) <= 12 );
    ASSERT ( abs ( (int) b - 50 ) <= 12 );
    IFreeImage ( back );
  }
  else {
    /* not compiled in: a clean error, and no image produced */
    ASSERT ( back == NULL );
  }
  IFreeImage ( im );
  PASS ();
}

TEST webp_alpha_roundtrip ( void )
{
  IImage im = ICreateImage ( 16, 16, IOPTION_ALPHA );
  IError wr, rd;
  IImage back;
  int x, y;

  for ( y = 0; y < 16; y++ )
    for ( x = 0; x < 16; x++ )
      ISetPixelAlpha ( im, x, y, 10, 20, 30, 128 );

  back = roundtrip ( im, IFORMAT_WEBP, &wr, &rd );
  if ( wr == INoError ) {
    unsigned int a = 0;
    ASSERT_EQ ( INoError, rd );
    ASSERT ( back != NULL );
    IGetPixelAlpha ( back, 8, 8, NULL, NULL, NULL, &a );
    ASSERT ( abs ( (int) a - 128 ) <= 12 );
    IFreeImage ( back );
  }
  else {
    ASSERT ( back == NULL );
  }
  IFreeImage ( im );
  PASS ();
}

/* AVIF round-trip (when libavif is compiled in). Lossy, so a solid colour is
   checked with tolerance; skip-clean when the codec is absent. */
TEST avif_roundtrip ( void )
{
  IImage im = ICreateImage ( 16, 16, IOPTION_NONE );
  IError wr, rd;
  IImage back;
  int x, y;

  for ( y = 0; y < 16; y++ )
    for ( x = 0; x < 16; x++ )
      ISetPixel ( im, x, y, 200, 100, 50 );

  back = roundtrip ( im, IFORMAT_AVIF, &wr, &rd );
  if ( wr == INoError ) {
    unsigned int r, g, b;
    ASSERT_EQ ( INoError, rd );
    ASSERT ( back != NULL );
    ASSERT_EQ ( 16, px_width ( back ) );
    ASSERT_EQ ( 16, px_height ( back ) );
    IGetPixel ( back, 8, 8, &r, &g, &b );
    ASSERT ( abs ( (int) r - 200 ) <= 16 );
    ASSERT ( abs ( (int) g - 100 ) <= 16 );
    ASSERT ( abs ( (int) b - 50 ) <= 16 );
    IFreeImage ( back );
  }
  else {
    ASSERT ( back == NULL );
  }
  IFreeImage ( im );
  PASS ();
}

TEST avif_alpha_roundtrip ( void )
{
  IImage im = ICreateImage ( 16, 16, IOPTION_ALPHA );
  IError wr, rd;
  IImage back;
  int x, y;

  for ( y = 0; y < 16; y++ )
    for ( x = 0; x < 16; x++ )
      ISetPixelAlpha ( im, x, y, 10, 20, 30, 128 );

  back = roundtrip ( im, IFORMAT_AVIF, &wr, &rd );
  if ( wr == INoError ) {
    unsigned int a = 0;
    ASSERT_EQ ( INoError, rd );
    ASSERT ( back != NULL );
    IGetPixelAlpha ( back, 8, 8, NULL, NULL, NULL, &a );
    ASSERT ( abs ( (int) a - 128 ) <= 16 );
    IFreeImage ( back );
  }
  else {
    ASSERT ( back == NULL );
  }
  IFreeImage ( im );
  PASS ();
}

SUITE ( formats )
{
  RUN_TEST ( ppm_roundtrip );
  RUN_TEST ( pgm_roundtrip_greyscale );
  RUN_TEST ( bmp_roundtrip );
  RUN_TEST ( garbage_ppm_rejected );
  RUN_TEST ( truncated_ppm_rejected );
  RUN_TEST ( reduce_colors_caps_palette );
  RUN_TEST ( reduce_colors_preserves_few );
  RUN_TEST ( reduce_colors_rejects_bad_handle );
  RUN_TEST ( dither_diffuses_gradient );
  RUN_TEST ( dither_rejects_bad_handle );
  RUN_TEST ( gif_write_quantizes );
  RUN_TEST ( optional_codec_write_is_clean );
  RUN_TEST ( webp_roundtrip );
  RUN_TEST ( webp_alpha_roundtrip );
  RUN_TEST ( avif_roundtrip );
  RUN_TEST ( avif_alpha_roundtrip );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( formats );
  GREATEST_MAIN_END ();
}
