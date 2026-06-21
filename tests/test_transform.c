/* SPDX-License-Identifier: GPL-2.0-only */
/* Geometric transforms (ITransform.c), checked through the public
   ISetPixel/IGetPixel accessors and IImageWidth/IImageHeight. */

#include <Ilib.h>
#include "greatest.h"

/* Convenience: red component of a pixel. */
static unsigned int red_at ( IImage im, int x, int y )
{
  unsigned int r = 0;
  IGetPixel ( im, x, y, &r, NULL, NULL );
  return ( r );
}

/* Mark pixel (x,y) with a unique red value so we can track where it moves. */
static void mark ( IImage im, int x, int y, unsigned int v )
{
  ISetPixel ( im, x, y, v, 0, 0 );
}

TEST flip_reverses_rows ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE );

  mark ( im, 0, 0, 10 ); /* top row */
  mark ( im, 0, 1, 20 ); /* bottom row */
  ASSERT_EQ ( INoError, IFlip ( im ) );
  ASSERT_EQ ( 20, red_at ( im, 0, 0 ) );
  ASSERT_EQ ( 10, red_at ( im, 0, 1 ) );

  IFreeImage ( im );
  PASS ();
}

TEST flop_reverses_columns ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE );

  mark ( im, 0, 0, 10 ); /* left */
  mark ( im, 2, 0, 30 ); /* right */
  ASSERT_EQ ( INoError, IFlop ( im ) );
  ASSERT_EQ ( 30, red_at ( im, 0, 0 ) );
  ASSERT_EQ ( 10, red_at ( im, 2, 0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_90_clockwise ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE ); /* W=3, H=2 */

  mark ( im, 0, 0, 10 ); /* top-left */
  mark ( im, 2, 0, 30 ); /* top-right */
  ASSERT_EQ ( INoError, IRotate ( im, 90 ) );

  /* Dimensions swap to 2x3. */
  ASSERT_EQ ( 2, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 3, (int) IImageHeight ( im ) );
  /* CW: old top-left -> new top-right; old top-right -> new bottom-right. */
  ASSERT_EQ ( 10, red_at ( im, 1, 0 ) );
  ASSERT_EQ ( 30, red_at ( im, 1, 2 ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_270_counterclockwise ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE );

  mark ( im, 0, 0, 10 ); /* top-left */
  ASSERT_EQ ( INoError, IRotate ( im, 270 ) );
  ASSERT_EQ ( 2, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 3, (int) IImageHeight ( im ) );
  /* CCW: old top-left -> new bottom-left. */
  ASSERT_EQ ( 10, red_at ( im, 0, 2 ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_180_reverses ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE );

  mark ( im, 0, 0, 10 );
  ASSERT_EQ ( INoError, IRotate ( im, 180 ) );
  ASSERT_EQ ( 3, (int) IImageWidth ( im ) ); /* dims unchanged */
  ASSERT_EQ ( 2, (int) IImageHeight ( im ) );
  /* (0,0) maps to the opposite corner (2,1). */
  ASSERT_EQ ( 10, red_at ( im, 2, 1 ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_full_turn_is_noop ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE );

  mark ( im, 1, 0, 42 );
  ASSERT_EQ ( INoError, IRotate ( im, 360 ) );
  ASSERT_EQ ( 42, red_at ( im, 1, 0 ) );
  ASSERT_EQ ( INoError, IRotate ( im, 0 ) );
  ASSERT_EQ ( 42, red_at ( im, 1, 0 ) );

  IFreeImage ( im );
  PASS ();
}

TEST rotate_negative_normalizes ( void )
{
  IImage a = ICreateImage ( 3, 2, IOPTION_NONE );
  IImage b = ICreateImage ( 3, 2, IOPTION_NONE );

  mark ( a, 0, 0, 77 );
  mark ( b, 0, 0, 77 );
  /* -90 should equal +270. */
  IRotate ( a, -90 );
  IRotate ( b, 270 );
  ASSERT_EQ ( red_at ( a, 0, 2 ), red_at ( b, 0, 2 ) );

  IFreeImage ( a );
  IFreeImage ( b );
  PASS ();
}

TEST rotate_rejects_non_multiple ( void )
{
  IImage im = ICreateImage ( 3, 2, IOPTION_NONE );

  ASSERT_EQ ( IInvalidArgument, IRotate ( im, 45 ) );

  IFreeImage ( im );
  PASS ();
}

TEST crop_resizes_and_keeps_content ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  mark ( im, 1, 1, 99 );
  mark ( im, 2, 2, 88 );
  ASSERT_EQ ( INoError, ICrop ( im, 1, 1, 2, 2 ) );
  ASSERT_EQ ( 2, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 2, (int) IImageHeight ( im ) );
  ASSERT_EQ ( 99, red_at ( im, 0, 0 ) );
  ASSERT_EQ ( 88, red_at ( im, 1, 1 ) );

  IFreeImage ( im );
  PASS ();
}

TEST crop_rejects_out_of_bounds ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  ASSERT_EQ ( IInvalidArgument, ICrop ( im, 2, 2, 4, 4 ) ); /* exceeds right */
  ASSERT_EQ ( IInvalidArgument, ICrop ( im, -1, 0, 2, 2 ) );
  ASSERT_EQ ( IInvalidArgument, ICrop ( im, 0, 0, 0, 2 ) ); /* zero size */

  IFreeImage ( im );
  PASS ();
}

TEST works_on_rgba ( void )
{
  IImage im = ICreateImage ( 2, 2, IOPTION_ALPHA );
  unsigned int a = 0;

  ISetPixelAlpha ( im, 0, 0, 10, 20, 30, 111 );
  ASSERT_EQ ( INoError, IRotate ( im, 90 ) );
  /* (0,0) -> (h-1-0,0) = (1,0) for a 2x2 image. */
  IGetPixelAlpha ( im, 1, 0, NULL, NULL, NULL, &a );
  ASSERT_EQ ( 111, a );

  IFreeImage ( im );
  PASS ();
}

TEST transforms_reject_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidImage, IFlip ( NULL ) );
  ASSERT_EQ ( IInvalidImage, IFlop ( NULL ) );
  ASSERT_EQ ( IInvalidImage, IRotate ( NULL, 90 ) );
  ASSERT_EQ ( IInvalidImage, ICrop ( NULL, 0, 0, 1, 1 ) );
  PASS ();
}

SUITE ( transform )
{
  RUN_TEST ( flip_reverses_rows );
  RUN_TEST ( flop_reverses_columns );
  RUN_TEST ( rotate_90_clockwise );
  RUN_TEST ( rotate_270_counterclockwise );
  RUN_TEST ( rotate_180_reverses );
  RUN_TEST ( rotate_full_turn_is_noop );
  RUN_TEST ( rotate_negative_normalizes );
  RUN_TEST ( rotate_rejects_non_multiple );
  RUN_TEST ( crop_resizes_and_keeps_content );
  RUN_TEST ( crop_rejects_out_of_bounds );
  RUN_TEST ( works_on_rgba );
  RUN_TEST ( transforms_reject_bad_handle );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( transform );
  GREATEST_MAIN_END ();
}
