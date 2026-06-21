/* SPDX-License-Identifier: GPL-2.0-only */
/* Whole-image point operations (IFilter.c), checked through the public
   IGetPixel/ISetPixel accessors. */

#include <Ilib.h>
#include "greatest.h"

#define W 4
#define H 4

/* Fill the whole image with one RGB color. */
static void fill ( IImage im, unsigned int r, unsigned int g, unsigned int b )
{
  int x, y;
  for ( y = 0; y < H; y++ )
    for ( x = 0; x < W; x++ )
      ISetPixel ( im, x, y, r, g, b );
}

TEST greyscale_desaturates ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r, g, b;

  fill ( im, 255, 0, 0 ); /* pure red -> luma 0.299*255 ~= 76 */
  ASSERT_EQ ( INoError, IGreyscale ( im ) );
  IGetPixel ( im, 1, 1, &r, &g, &b );
  ASSERT_EQ ( r, g );
  ASSERT_EQ ( g, b );
  ASSERT ( r > 70 && r < 82 );

  IFreeImage ( im );
  PASS ();
}

TEST negate_inverts ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r, g, b;

  fill ( im, 10, 20, 30 );
  ASSERT_EQ ( INoError, INegate ( im ) );
  IGetPixel ( im, 0, 0, &r, &g, &b );
  ASSERT_EQ ( 245, r );
  ASSERT_EQ ( 235, g );
  ASSERT_EQ ( 225, b );

  IFreeImage ( im );
  PASS ();
}

TEST brightness_increases_value ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r0, r1;

  fill ( im, 100, 100, 100 );
  IGetPixel ( im, 0, 0, &r0, NULL, NULL );
  ASSERT_EQ ( INoError, IBrightnessContrast ( im, 50, 0 ) );
  IGetPixel ( im, 0, 0, &r1, NULL, NULL );
  ASSERT ( r1 > r0 );

  IFreeImage ( im );
  PASS ();
}

TEST contrast_pushes_away_from_mid ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int dark, light;

  /* A dark pixel should get darker, a light pixel lighter, under +contrast. */
  ISetPixel ( im, 0, 0, 100, 100, 100 );
  ISetPixel ( im, 1, 0, 160, 160, 160 );
  ASSERT_EQ ( INoError, IBrightnessContrast ( im, 0, 60 ) );
  IGetPixel ( im, 0, 0, &dark, NULL, NULL );
  IGetPixel ( im, 1, 0, &light, NULL, NULL );
  ASSERT ( dark < 100 );
  ASSERT ( light > 160 );

  IFreeImage ( im );
  PASS ();
}

TEST brightness_contrast_clamps_range ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );

  fill ( im, 128, 128, 128 );
  /* Out-of-range arguments are clamped, not rejected. */
  ASSERT_EQ ( INoError, IBrightnessContrast ( im, 1000, -1000 ) );

  IFreeImage ( im );
  PASS ();
}

TEST gamma_identity_and_brighten ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r;

  fill ( im, 64, 64, 64 );
  ASSERT_EQ ( INoError, IGamma ( im, 1.0 ) ); /* identity */
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT_EQ ( 64, r );

  ASSERT_EQ ( INoError, IGamma ( im, 2.0 ) ); /* >1 brightens midtones */
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT ( r > 64 );

  IFreeImage ( im );
  PASS ();
}

TEST gamma_rejects_nonpositive ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );

  ASSERT_EQ ( IInvalidArgument, IGamma ( im, 0.0 ) );
  ASSERT_EQ ( IInvalidArgument, IGamma ( im, -1.0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST threshold_makes_black_and_white ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r;

  ISetPixel ( im, 0, 0, 50, 50, 50 );    /* below */
  ISetPixel ( im, 1, 0, 200, 200, 200 ); /* at/above */
  ASSERT_EQ ( INoError, IThreshold ( im, 128 ) );
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT_EQ ( 0, r );
  IGetPixel ( im, 1, 0, &r, NULL, NULL );
  ASSERT_EQ ( 255, r );

  IFreeImage ( im );
  PASS ();
}

TEST threshold_rejects_out_of_range ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );

  ASSERT_EQ ( IInvalidArgument, IThreshold ( im, 256 ) );

  IFreeImage ( im );
  PASS ();
}

TEST filters_reject_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidImage, IGreyscale ( NULL ) );
  ASSERT_EQ ( IInvalidImage, INegate ( NULL ) );
  ASSERT_EQ ( IInvalidImage, IBrightnessContrast ( NULL, 0, 0 ) );
  ASSERT_EQ ( IInvalidImage, IGamma ( NULL, 1.0 ) );
  ASSERT_EQ ( IInvalidImage, IThreshold ( NULL, 128 ) );
  PASS ();
}

TEST preserves_alpha ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_ALPHA );
  unsigned int a;

  ISetPixelAlpha ( im, 0, 0, 10, 20, 30, 123 );
  ASSERT_EQ ( INoError, INegate ( im ) );
  IGetPixelAlpha ( im, 0, 0, NULL, NULL, NULL, &a );
  ASSERT_EQ ( 123, a ); /* alpha untouched by a colour filter */

  IFreeImage ( im );
  PASS ();
}

TEST normalize_stretches_contrast ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int lo, hi;

  /* A low-contrast image (values 100..150) should stretch to 0..255. */
  fill ( im, 100, 100, 100 );
  ISetPixel ( im, 1, 0, 150, 150, 150 );
  ASSERT_EQ ( INoError, INormalize ( im ) );
  IGetPixel ( im, 0, 0, &lo, NULL, NULL );
  IGetPixel ( im, 1, 0, &hi, NULL, NULL );
  ASSERT_EQ ( 0, lo );
  ASSERT_EQ ( 255, hi );

  IFreeImage ( im );
  PASS ();
}

TEST normalize_flat_is_noop ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r;

  fill ( im, 90, 90, 90 );
  ASSERT_EQ ( INoError, INormalize ( im ) );
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT_EQ ( 90, r ); /* flat: unchanged, not divide-by-zero */

  IFreeImage ( im );
  PASS ();
}

TEST sepia_warms_grey ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  unsigned int r, g, b;

  fill ( im, 128, 128, 128 );
  ASSERT_EQ ( INoError, ISepia ( im ) );
  IGetPixel ( im, 0, 0, &r, &g, &b );
  /* Sepia of mid-grey is a warm brown: red >= green >= blue. */
  ASSERT ( r >= g );
  ASSERT ( g >= b );
  ASSERT ( r > b );

  IFreeImage ( im );
  PASS ();
}

TEST opacity_scales_alpha ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_ALPHA );
  unsigned int a;

  ISetPixelAlpha ( im, 0, 0, 10, 20, 30, 200 );
  ASSERT_EQ ( INoError, IOpacity ( im, 0.5 ) );
  IGetPixelAlpha ( im, 0, 0, NULL, NULL, NULL, &a );
  ASSERT_EQ ( 100, a );

  IFreeImage ( im );
  PASS ();
}

TEST opacity_clamps_and_rejects ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_ALPHA );
  unsigned int a;

  ISetPixelAlpha ( im, 0, 0, 0, 0, 0, 200 );
  ASSERT_EQ ( INoError, IOpacity ( im, 10.0 ) ); /* clamps to 255 */
  IGetPixelAlpha ( im, 0, 0, NULL, NULL, NULL, &a );
  ASSERT_EQ ( 255, a );
  ASSERT_EQ ( IInvalidArgument, IOpacity ( im, -1.0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST opacity_noop_on_rgb ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );

  /* No alpha channel: a clean no-op, not an error. */
  ASSERT_EQ ( INoError, IOpacity ( im, 0.5 ) );

  IFreeImage ( im );
  PASS ();
}

TEST polish_ops_reject_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidImage, INormalize ( NULL ) );
  ASSERT_EQ ( IInvalidImage, ISepia ( NULL ) );
  ASSERT_EQ ( IInvalidImage, IOpacity ( NULL, 1.0 ) );
  PASS ();
}

TEST greyscale_on_grey_image_ok ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_GREYSCALE );

  fill ( im, 80, 80, 80 );
  ASSERT_EQ ( INoError, IGreyscale ( im ) ); /* no-op, must not error */

  IFreeImage ( im );
  PASS ();
}

SUITE ( filter )
{
  RUN_TEST ( greyscale_desaturates );
  RUN_TEST ( negate_inverts );
  RUN_TEST ( brightness_increases_value );
  RUN_TEST ( contrast_pushes_away_from_mid );
  RUN_TEST ( brightness_contrast_clamps_range );
  RUN_TEST ( gamma_identity_and_brighten );
  RUN_TEST ( gamma_rejects_nonpositive );
  RUN_TEST ( threshold_makes_black_and_white );
  RUN_TEST ( threshold_rejects_out_of_range );
  RUN_TEST ( filters_reject_bad_handle );
  RUN_TEST ( preserves_alpha );
  RUN_TEST ( greyscale_on_grey_image_ok );
  RUN_TEST ( normalize_stretches_contrast );
  RUN_TEST ( normalize_flat_is_noop );
  RUN_TEST ( sepia_warms_grey );
  RUN_TEST ( opacity_scales_alpha );
  RUN_TEST ( opacity_clamps_and_rejects );
  RUN_TEST ( opacity_noop_on_rgb );
  RUN_TEST ( polish_ops_reject_bad_handle );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( filter );
  GREATEST_MAIN_END ();
}
