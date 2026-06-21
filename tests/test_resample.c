/* SPDX-License-Identifier: GPL-2.0-only */
/* Resampling transforms (IResample.c): bilinear resize and arbitrary-angle
   rotation, checked through the public accessors. */

#include <stdlib.h>
#include <Ilib.h>
#include "greatest.h"

static unsigned int red_at ( IImage im, int x, int y )
{
  unsigned int r = 0;
  IGetPixel ( im, x, y, &r, NULL, NULL );
  return ( r );
}

TEST resize_changes_dimensions ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  ASSERT_EQ ( INoError, IResize ( im, 8, 2 ) );
  ASSERT_EQ ( 8, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 2, (int) IImageHeight ( im ) );

  IFreeImage ( im );
  PASS ();
}

TEST resize_preserves_solid_color ( void )
{
  IImage im = ICreateImage ( 3, 3, IOPTION_NONE );
  int x, y;
  unsigned int r, g, b;

  for ( y = 0; y < 3; y++ )
    for ( x = 0; x < 3; x++ )
      ISetPixel ( im, x, y, 10, 20, 30 );

  ASSERT_EQ ( INoError, IResize ( im, 7, 5 ) ); /* upscale */
  IGetPixel ( im, 3, 2, &r, &g, &b );
  ASSERT_EQ ( 10, r );
  ASSERT_EQ ( 20, g );
  ASSERT_EQ ( 30, b );

  IFreeImage ( im );
  PASS ();
}

TEST resize_keeps_corners ( void )
{
  IImage im = ICreateImage ( 2, 2, IOPTION_NONE );

  ISetPixel ( im, 0, 0, 200, 0, 0 ); /* top-left red */
  ISetPixel ( im, 1, 1, 50, 0, 0 );  /* bottom-right */
  ASSERT_EQ ( INoError, IResize ( im, 6, 6 ) );
  /* Corner pixels sample the (clamped) source corners exactly. */
  ASSERT_EQ ( 200, red_at ( im, 0, 0 ) );
  ASSERT_EQ ( 50, red_at ( im, 5, 5 ) );

  IFreeImage ( im );
  PASS ();
}

TEST resize_downscale ( void )
{
  IImage im = ICreateImage ( 8, 8, IOPTION_NONE );

  ASSERT_EQ ( INoError, IResize ( im, 2, 2 ) );
  ASSERT_EQ ( 2, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 2, (int) IImageHeight ( im ) );

  IFreeImage ( im );
  PASS ();
}

TEST resize_rejects_zero ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  ASSERT_EQ ( IInvalidArgument, IResize ( im, 0, 4 ) );
  ASSERT_EQ ( IInvalidArgument, IResize ( im, 4, 0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST resize_preserves_alpha ( void )
{
  IImage im = ICreateImage ( 3, 3, IOPTION_ALPHA );
  int x, y;
  unsigned int a;

  for ( y = 0; y < 3; y++ )
    for ( x = 0; x < 3; x++ )
      ISetPixelAlpha ( im, x, y, 0, 0, 0, 100 );
  ASSERT_EQ ( INoError, IResize ( im, 6, 6 ) );
  IGetPixelAlpha ( im, 3, 3, NULL, NULL, NULL, &a );
  ASSERT_EQ ( 100, a );

  IFreeImage ( im );
  PASS ();
}

/* A black/white checkerboard downscaled to 1x1: area-averaging yields mid-grey,
   while nearest/point sampling yields one extreme. */
static IImage make_checker ( int n )
{
  IImage im = ICreateImage ( n, n, IOPTION_NONE );
  int x, y;
  for ( y = 0; y < n; y++ )
    for ( x = 0; x < n; x++ ) {
      unsigned int v = ( ( x + y ) % 2 == 0 ) ? 255 : 0;
      ISetPixel ( im, x, y, v, v, v );
    }
  return ( im );
}

TEST area_downscale_averages ( void )
{
  IImage im = make_checker ( 8 );
  unsigned int r;

  ASSERT_EQ ( INoError, IResizeFiltered ( im, 1, 1, IRESIZE_AREA ) );
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT ( r > 118 && r < 138 ); /* ~127 grey */

  IFreeImage ( im );
  PASS ();
}

TEST nearest_downscale_does_not_average ( void )
{
  IImage im = make_checker ( 8 );
  unsigned int r;

  ASSERT_EQ ( INoError, IResizeFiltered ( im, 1, 1, IRESIZE_NEAREST ) );
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT ( r == 0 || r == 255 ); /* a single source pixel, not a blend */

  IFreeImage ( im );
  PASS ();
}

TEST auto_shrinks_with_area ( void )
{
  IImage im = make_checker ( 8 );
  unsigned int r;

  /* AUTO should choose area-averaging when shrinking. */
  ASSERT_EQ ( INoError, IResizeFiltered ( im, 1, 1, IRESIZE_AUTO ) );
  IGetPixel ( im, 0, 0, &r, NULL, NULL );
  ASSERT ( r > 118 && r < 138 );

  IFreeImage ( im );
  PASS ();
}

TEST bicubic_preserves_solid_and_dims ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  int x, y;
  unsigned int r, g, b;

  for ( y = 0; y < 4; y++ )
    for ( x = 0; x < 4; x++ )
      ISetPixel ( im, x, y, 100, 150, 200 );
  ASSERT_EQ ( INoError, IResizeFiltered ( im, 16, 16, IRESIZE_BICUBIC ) );
  ASSERT_EQ ( 16, (int) IImageWidth ( im ) );
  IGetPixel ( im, 8, 8, &r, &g, &b );
  ASSERT ( abs ( (int) r - 100 ) <= 1 );
  ASSERT ( abs ( (int) g - 150 ) <= 1 );
  ASSERT ( abs ( (int) b - 200 ) <= 1 );

  IFreeImage ( im );
  PASS ();
}

TEST resize_filtered_rejects_zero ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  ASSERT_EQ ( IInvalidArgument, IResizeFiltered ( im, 0, 4, IRESIZE_AREA ) );
  ASSERT_EQ ( IInvalidImage, IResizeFiltered ( NULL, 4, 4, IRESIZE_AUTO ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_angle_zero_is_identity ( void )
{
  IImage im = ICreateImage ( 4, 3, IOPTION_NONE );
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetPixel ( im, 1, 1, 123, 0, 0 );
  ASSERT_EQ ( INoError, IRotateAngle ( im, 0.0, black ) );
  ASSERT_EQ ( 4, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 3, (int) IImageHeight ( im ) );
  ASSERT_EQ ( 123, red_at ( im, 1, 1 ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_angle_90_clockwise ( void )
{
  IImage im = ICreateImage ( 3, 3, IOPTION_NONE );
  IColor black = IAllocColor ( 0, 0, 0 );
  int x, y;

  for ( y = 0; y < 3; y++ )
    for ( x = 0; x < 3; x++ )
      ISetPixel ( im, x, y, 0, 0, 0 );
  ISetPixel ( im, 0, 0, 200, 0, 0 ); /* top-left marker */

  ASSERT_EQ ( INoError, IRotateAngle ( im, 90.0, black ) );
  ASSERT_EQ ( 3, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 3, (int) IImageHeight ( im ) );
  /* Clockwise: top-left -> top-right, just like IRotate(90). */
  ASSERT ( red_at ( im, 2, 0 ) > 150 );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_angle_45_grows_and_fills_background ( void )
{
  IImage im = ICreateImage ( 3, 3, IOPTION_NONE );
  IColor green = IAllocColor ( 0, 255, 0 );
  unsigned int r, g, b;

  ASSERT_EQ ( INoError, IRotateAngle ( im, 45.0, green ) );
  /* Bounding box grows. */
  ASSERT ( (int) IImageWidth ( im ) > 3 );
  ASSERT ( (int) IImageHeight ( im ) > 3 );
  /* A corner is outside the rotated source -> background colour. */
  IGetPixel ( im, 0, 0, &r, &g, &b );
  ASSERT_EQ ( 0, r );
  ASSERT_EQ ( 255, g );
  ASSERT_EQ ( 0, b );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_angle_rejects_bad_color ( void )
{
  IImage im = ICreateImage ( 3, 3, IOPTION_NONE );

  /* A wildly out-of-range colour index does not resolve. */
  ASSERT_EQ ( IInvalidColor, IRotateAngle ( im, 30.0, 999999u ) );

  IFreeImage ( im );
  PASS ();
}

TEST resample_reject_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidImage, IResize ( NULL, 4, 4 ) );
  ASSERT_EQ ( IInvalidImage, IRotateAngle ( NULL, 30.0, 0 ) );
  PASS ();
}

SUITE ( resample )
{
  RUN_TEST ( resize_changes_dimensions );
  RUN_TEST ( resize_preserves_solid_color );
  RUN_TEST ( resize_keeps_corners );
  RUN_TEST ( resize_downscale );
  RUN_TEST ( resize_rejects_zero );
  RUN_TEST ( resize_preserves_alpha );
  RUN_TEST ( area_downscale_averages );
  RUN_TEST ( nearest_downscale_does_not_average );
  RUN_TEST ( auto_shrinks_with_area );
  RUN_TEST ( bicubic_preserves_solid_and_dims );
  RUN_TEST ( resize_filtered_rejects_zero );
  RUN_TEST ( rotate_angle_zero_is_identity );
  RUN_TEST ( rotate_angle_90_clockwise );
  RUN_TEST ( rotate_angle_45_grows_and_fills_background );
  RUN_TEST ( rotate_angle_rejects_bad_color );
  RUN_TEST ( resample_reject_bad_handle );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( resample );
  GREATEST_MAIN_END ();
}
