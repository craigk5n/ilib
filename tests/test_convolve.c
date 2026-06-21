/* SPDX-License-Identifier: GPL-2.0-only */
/* Convolution / area filters (IConvolve.c), checked through the public
   ISetPixel/IGetPixel accessors. */

#include <Ilib.h>
#include "greatest.h"

static unsigned int red_at ( IImage im, int x, int y )
{
  unsigned int r = 0;
  IGetPixel ( im, x, y, &r, NULL, NULL );
  return ( r );
}

/* Fill the whole image with one grey level. */
static void fill_grey ( IImage im, int w, int h, unsigned int v )
{
  int x, y;
  for ( y = 0; y < h; y++ )
    for ( x = 0; x < w; x++ )
      ISetPixel ( im, x, y, v, v, v );
}

TEST blur_spreads_a_bright_pixel ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_NONE );

  fill_grey ( im, 5, 5, 0 );
  ISetPixel ( im, 2, 2, 255, 255, 255 ); /* lone bright centre */
  ASSERT_EQ ( INoError, IBlur ( im, 1 ) );

  /* Centre is averaged down (255/9 ~= 28), an orthogonal neighbour gains
     light, and a far corner (outside the 3x3 reach) stays dark. */
  ASSERT ( red_at ( im, 2, 2 ) > 20 && red_at ( im, 2, 2 ) < 40 );
  ASSERT ( red_at ( im, 2, 1 ) > 0 );
  ASSERT_EQ ( 0, red_at ( im, 0, 0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST blur_radius_zero_is_noop ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  fill_grey ( im, 4, 4, 50 );
  ISetPixel ( im, 1, 1, 200, 200, 200 );
  ASSERT_EQ ( INoError, IBlur ( im, 0 ) );
  ASSERT_EQ ( 200, red_at ( im, 1, 1 ) );

  IFreeImage ( im );
  PASS ();
}

TEST gaussian_blur_spreads_and_rejects_bad_sigma ( void )
{
  IImage im = ICreateImage ( 7, 7, IOPTION_NONE );

  fill_grey ( im, 7, 7, 0 );
  ISetPixel ( im, 3, 3, 255, 255, 255 );
  ASSERT_EQ ( INoError, IGaussianBlur ( im, 1.0 ) );
  ASSERT ( red_at ( im, 3, 3 ) < 255 ); /* centre softened */
  ASSERT ( red_at ( im, 3, 2 ) > 0 );   /* neighbour gained light */

  ASSERT_EQ ( IInvalidArgument, IGaussianBlur ( im, 0.0 ) );
  ASSERT_EQ ( IInvalidArgument, IGaussianBlur ( im, -1.0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST sharpen_leaves_flat_unchanged ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_NONE );

  fill_grey ( im, 5, 5, 120 );
  ASSERT_EQ ( INoError, ISharpen ( im ) );
  /* The sharpen kernel sums to 1, so a flat region is unchanged. */
  ASSERT_EQ ( 120, red_at ( im, 2, 2 ) );

  IFreeImage ( im );
  PASS ();
}

TEST edge_detect_blacks_out_flat ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_NONE );

  fill_grey ( im, 5, 5, 130 );
  ASSERT_EQ ( INoError, IEdgeDetect ( im ) );
  /* The Laplacian sums to 0, so a flat region becomes black. */
  ASSERT_EQ ( 0, red_at ( im, 2, 2 ) );

  IFreeImage ( im );
  PASS ();
}

TEST emboss_flattens_to_mid_grey ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_NONE );

  fill_grey ( im, 5, 5, 90 );
  ASSERT_EQ ( INoError, IEmboss ( im ) );
  /* Kernel sums to 0 with bias 128, so a flat region becomes mid-grey. */
  ASSERT_EQ ( 128, red_at ( im, 2, 2 ) );

  IFreeImage ( im );
  PASS ();
}

TEST convolve_identity_is_noop ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  /* clang-format off */
  const double identity[9] = {
    0, 0, 0,
    0, 1, 0,
    0, 0, 0
  };
  /* clang-format on */

  ISetPixel ( im, 1, 2, 77, 0, 0 );
  ASSERT_EQ ( INoError, IConvolve ( im, identity, 3, 1.0, 0.0 ) );
  ASSERT_EQ ( 77, red_at ( im, 1, 2 ) );

  IFreeImage ( im );
  PASS ();
}

TEST convolve_rejects_bad_args ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  const double k4[16] = { 0 };
  const double k3[9] = { 0 };

  ASSERT_EQ ( IInvalidArgument, IConvolve ( im, NULL, 3, 1.0, 0.0 ) );
  ASSERT_EQ ( IInvalidArgument, IConvolve ( im, k4, 4, 1.0, 0.0 ) ); /* even */
  ASSERT_EQ ( IInvalidArgument, IConvolve ( im, k3, 0, 1.0, 0.0 ) ); /* zero */

  IFreeImage ( im );
  PASS ();
}

TEST convolve_filters_reject_bad_handle ( void )
{
  const double k[9] = { 0 };
  ASSERT_EQ ( IInvalidImage, IConvolve ( NULL, k, 3, 1.0, 0.0 ) );
  ASSERT_EQ ( IInvalidImage, IBlur ( NULL, 1 ) );
  ASSERT_EQ ( IInvalidImage, IGaussianBlur ( NULL, 1.0 ) );
  ASSERT_EQ ( IInvalidImage, ISharpen ( NULL ) );
  ASSERT_EQ ( IInvalidImage, IEdgeDetect ( NULL ) );
  ASSERT_EQ ( IInvalidImage, IEmboss ( NULL ) );
  PASS ();
}

TEST blur_preserves_alpha ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_ALPHA );
  unsigned int a = 0;

  ISetPixelAlpha ( im, 1, 1, 200, 100, 50, 123 );
  ASSERT_EQ ( INoError, IBlur ( im, 1 ) );
  IGetPixelAlpha ( im, 1, 1, NULL, NULL, NULL, &a );
  ASSERT_EQ ( 123, a ); /* alpha channel left untouched */

  IFreeImage ( im );
  PASS ();
}

SUITE ( convolve )
{
  RUN_TEST ( blur_spreads_a_bright_pixel );
  RUN_TEST ( blur_radius_zero_is_noop );
  RUN_TEST ( gaussian_blur_spreads_and_rejects_bad_sigma );
  RUN_TEST ( sharpen_leaves_flat_unchanged );
  RUN_TEST ( edge_detect_blacks_out_flat );
  RUN_TEST ( emboss_flattens_to_mid_grey );
  RUN_TEST ( convolve_identity_is_noop );
  RUN_TEST ( convolve_rejects_bad_args );
  RUN_TEST ( convolve_filters_reject_bad_handle );
  RUN_TEST ( blur_preserves_alpha );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( convolve );
  GREATEST_MAIN_END ();
}
