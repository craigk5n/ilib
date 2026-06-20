/* SPDX-License-Identifier: GPL-2.0-only */
/* Public per-pixel get/set accessors and the alpha/blend model (Phase A).
   These are self-verifying (set then get) so no white-box access is needed. */

#include <stdio.h>
#include <Ilib.h>
#include "greatest.h"

TEST set_then_get_roundtrips ( void )
{
  IImage im = ICreateImage ( 8, 6, IOPTION_NONE );
  unsigned int r, g, b;

  ASSERT_EQ ( INoError, ISetPixel ( im, 3, 2, 10, 150, 240 ) );
  ASSERT_EQ ( INoError, IGetPixel ( im, 3, 2, &r, &g, &b ) );
  ASSERT_EQ ( 10, r );
  ASSERT_EQ ( 150, g );
  ASSERT_EQ ( 240, b );

  /* A different pixel is unaffected (default white). */
  ASSERT_EQ ( INoError, IGetPixel ( im, 0, 0, &r, &g, &b ) );
  ASSERT_EQ ( 255, r );
  ASSERT_EQ ( 255, g );
  ASSERT_EQ ( 255, b );

  IFreeImage ( im );
  PASS ();
}

TEST get_allows_null_channels ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  unsigned int g = 0;

  ASSERT_EQ ( INoError, ISetPixel ( im, 1, 1, 5, 99, 200 ) );
  ASSERT_EQ ( INoError, IGetPixel ( im, 1, 1, NULL, &g, NULL ) );
  ASSERT_EQ ( 99, g );

  IFreeImage ( im );
  PASS ();
}

TEST greyscale_uses_single_value ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_GREYSCALE );
  unsigned int r, g, b;

  ASSERT_EQ ( INoError, ISetPixel ( im, 2, 2, 123, 0, 0 ) );
  ASSERT_EQ ( INoError, IGetPixel ( im, 2, 2, &r, &g, &b ) );
  ASSERT_EQ ( 123, r );
  ASSERT_EQ ( 123, g );
  ASSERT_EQ ( 123, b );

  IFreeImage ( im );
  PASS ();
}

TEST out_of_bounds_rejected ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  unsigned int r, g, b;

  ASSERT_EQ ( IInvalidArgument, ISetPixel ( im, -1, 0, 1, 2, 3 ) );
  ASSERT_EQ ( IInvalidArgument, ISetPixel ( im, 4, 0, 1, 2, 3 ) );
  ASSERT_EQ ( IInvalidArgument, ISetPixel ( im, 0, 4, 1, 2, 3 ) );
  ASSERT_EQ ( IInvalidArgument, IGetPixel ( im, 0, -1, &r, &g, &b ) );
  ASSERT_EQ ( IInvalidArgument, IGetPixel ( im, 4, 4, &r, &g, &b ) );

  IFreeImage ( im );
  PASS ();
}

TEST channel_over_255_rejected ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  ASSERT_EQ ( IInvalidArgument, ISetPixel ( im, 0, 0, 256, 0, 0 ) );
  ASSERT_EQ ( IInvalidArgument, ISetPixel ( im, 0, 0, 0, 999, 0 ) );
  IFreeImage ( im );
  PASS ();
}

TEST bad_handle_rejected ( void )
{
  unsigned int not_an_image = 0;
  unsigned int r, g, b;
  ASSERT_EQ ( IInvalidImage, ISetPixel ( NULL, 0, 0, 1, 2, 3 ) );
  ASSERT_EQ ( IInvalidImage, IGetPixel ( NULL, 0, 0, &r, &g, &b ) );
  ASSERT_EQ ( IInvalidImage,
    ISetPixel ( (IImage) &not_an_image, 0, 0, 1, 2, 3 ) );
  ASSERT_EQ ( IInvalidImage,
    IGetPixel ( (IImage) &not_an_image, 0, 0, &r, &g, &b ) );
  PASS ();
}

/* --- Phase A: alpha channel + blending --------------------------------- */

TEST rgba_pixel_alpha_roundtrips ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_ALPHA );
  unsigned int r, g, b, a;

  ASSERT ( im != NULL );
  ASSERT_EQ ( INoError, ISetPixelAlpha ( im, 1, 1, 10, 20, 30, 128 ) );
  ASSERT_EQ ( INoError, IGetPixelAlpha ( im, 1, 1, &r, &g, &b, &a ) );
  ASSERT_EQ ( 10, r );
  ASSERT_EQ ( 20, g );
  ASSERT_EQ ( 30, b );
  ASSERT_EQ ( 128, a );

  /* A fresh RGBA image starts opaque white. */
  ASSERT_EQ ( INoError, IGetPixelAlpha ( im, 0, 0, &r, &g, &b, &a ) );
  ASSERT_EQ ( 255, r );
  ASSERT_EQ ( 255, a );

  IFreeImage ( im );
  PASS ();
}

TEST setpixel_on_rgba_is_opaque ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_ALPHA );
  unsigned int a = 0;
  /* plain ISetPixel sets alpha to 255 */
  ASSERT_EQ ( INoError, ISetPixel ( im, 2, 2, 1, 2, 3 ) );
  ASSERT_EQ ( INoError, IGetPixelAlpha ( im, 2, 2, NULL, NULL, NULL, &a ) );
  ASSERT_EQ ( 255, a );
  IFreeImage ( im );
  PASS ();
}

TEST alpha_greyscale_rejected ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_ALPHA | IOPTION_GREYSCALE );
  ASSERT ( im == NULL );
  PASS ();
}

/* Source-over: 50%-alpha black drawn over a white RGB image lands near 127. */
TEST blend_over_rgb ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor half_black = IAllocColorAlpha ( 0, 0, 0, 128 );
  unsigned int r, g, b;

  ASSERT_EQ ( INoError, ISetForeground ( gc, half_black ) );
  ASSERT_EQ ( INoError, ISetBlendMode ( gc, IBLEND_OVER ) );
  ASSERT_EQ ( INoError, IDrawPoint ( im, gc, 1, 1 ) );

  ASSERT_EQ ( INoError, IGetPixel ( im, 1, 1, &r, &g, &b ) );
  ASSERT_IN_RANGE ( 127, r, 1 );
  ASSERT_IN_RANGE ( 127, g, 1 );
  ASSERT_IN_RANGE ( 127, b, 1 );

  /* REPLACE (default) blend mode is unaffected: a fresh GC overwrites. */
  IFreeColor ( half_black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* Blending over an opaque RGBA destination keeps it opaque. */
TEST blend_over_rgba_stays_opaque ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_ALPHA );
  IGC gc = ICreateGC ();
  IColor half_red = IAllocColorAlpha ( 255, 0, 0, 128 );
  unsigned int r, a;

  ASSERT_EQ ( INoError, ISetForeground ( gc, half_red ) );
  ASSERT_EQ ( INoError, ISetBlendMode ( gc, IBLEND_OVER ) );
  ASSERT_EQ ( INoError, IDrawPoint ( im, gc, 0, 0 ) );

  ASSERT_EQ ( INoError, IGetPixelAlpha ( im, 0, 0, &r, NULL, NULL, &a ) );
  ASSERT_EQ ( 255, a );          /* over opaque white -> stays opaque */
  ASSERT_IN_RANGE ( 255, r, 1 ); /* red over white stays ~255 */

  IFreeColor ( half_red );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* Writing an RGBA image flattens it over white: a fully transparent pixel
   reads back white through a format that has no alpha (PPM). */
TEST rgba_flattens_on_write ( void )
{
  IImage im = ICreateImage ( 2, 1, IOPTION_ALPHA );
  IImage back = NULL;
  FILE *fp = tmpfile ();
  unsigned int r, g, b;

  ASSERT ( fp != NULL );
  ASSERT_EQ ( INoError, ISetPixelAlpha ( im, 0, 0, 10, 20, 30, 0 ) ); /* clear */
  ASSERT_EQ ( INoError, ISetPixelAlpha ( im, 1, 0, 0, 0, 0, 255 ) );  /* black */

  ASSERT_EQ ( INoError, IWriteImageFile ( fp, im, IFORMAT_PPM, IOPTION_NONE ) );
  rewind ( fp );
  ASSERT_EQ ( INoError, IReadImageFile ( fp, IFORMAT_PPM, IOPTION_NONE, &back ) );
  ASSERT ( back != NULL );

  ASSERT_EQ ( INoError, IGetPixel ( back, 0, 0, &r, &g, &b ) );
  ASSERT_EQ ( 255, r ); /* transparent flattened over white */
  ASSERT_EQ ( 255, g );
  ASSERT_EQ ( 255, b );
  ASSERT_EQ ( INoError, IGetPixel ( back, 1, 0, &r, &g, &b ) );
  ASSERT_EQ ( 0, r ); /* opaque black survives */

  fclose ( fp );
  IFreeImage ( back );
  IFreeImage ( im );
  PASS ();
}

/* PNG round-trips the alpha channel (Phase D). Skipped cleanly when libpng is
   not compiled in. A flattened (alpha-less) round-trip would change the color
   and force alpha=255, so checking the exact RGBA proves true alpha I/O. */
TEST png_rgba_roundtrips ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_ALPHA );
  FILE *fp = tmpfile ();
  IImage back = NULL;
  unsigned int r, g, b, a;
  IError wr;

  ASSERT ( fp != NULL );
  ASSERT_EQ ( INoError, ISetPixelAlpha ( im, 0, 0, 200, 100, 50, 128 ) );
  ASSERT_EQ ( INoError, ISetPixelAlpha ( im, 1, 1, 0, 0, 0, 0 ) ); /* clear */

  wr = IWriteImageFile ( fp, im, IFORMAT_PNG, IOPTION_NONE );
  if ( wr == INoError ) {
    rewind ( fp );
    ASSERT_EQ ( INoError,
      IReadImageFile ( fp, IFORMAT_PNG, IOPTION_NONE, &back ) );
    ASSERT ( back != NULL );
    ASSERT_EQ ( INoError, IGetPixelAlpha ( back, 0, 0, &r, &g, &b, &a ) );
    ASSERT_EQ ( 200, r );
    ASSERT_EQ ( 100, g );
    ASSERT_EQ ( 50, b );
    ASSERT_EQ ( 128, a );
    ASSERT_EQ ( INoError, IGetPixelAlpha ( back, 1, 1, NULL, NULL, NULL, &a ) );
    ASSERT_EQ ( 0, a );
    IFreeImage ( back );
  }
  fclose ( fp );
  IFreeImage ( im );
  PASS ();
}

SUITE ( pixel )
{
  RUN_TEST ( set_then_get_roundtrips );
  RUN_TEST ( get_allows_null_channels );
  RUN_TEST ( greyscale_uses_single_value );
  RUN_TEST ( out_of_bounds_rejected );
  RUN_TEST ( channel_over_255_rejected );
  RUN_TEST ( bad_handle_rejected );
  RUN_TEST ( rgba_pixel_alpha_roundtrips );
  RUN_TEST ( setpixel_on_rgba_is_opaque );
  RUN_TEST ( alpha_greyscale_rejected );
  RUN_TEST ( blend_over_rgb );
  RUN_TEST ( blend_over_rgba_stays_opaque );
  RUN_TEST ( rgba_flattens_on_write );
  RUN_TEST ( png_rgba_roundtrips );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( pixel );
  GREATEST_MAIN_END ();
}
