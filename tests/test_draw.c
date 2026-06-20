/* SPDX-License-Identifier: GPL-2.0-only */
/* Drawing primitives, checked with golden-pixel assertions via white-box
   access to the image buffer (see pixutil.h). */

#include <Ilib.h>
#include "pixutil.h"
#include "greatest.h"

#define W 10
#define H 10

TEST draw_point_sets_one_pixel ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor red = IAllocColor ( 255, 0, 0 );

  ISetForeground ( gc, red );
  ASSERT_EQ ( INoError, IDrawPoint ( im, gc, 5, 5 ) );

  /* the drawn pixel is red ... */
  ASSERT_EQ ( 255, px_r ( im, 5, 5 ) );
  ASSERT_EQ ( 0, px_g ( im, 5, 5 ) );
  ASSERT_EQ ( 0, px_b ( im, 5, 5 ) );
  /* ... and a neighbour is still the white background. */
  ASSERT_EQ ( 255, px_r ( im, 0, 0 ) );
  ASSERT_EQ ( 255, px_g ( im, 0, 0 ) );
  ASSERT_EQ ( 255, px_b ( im, 0, 0 ) );

  IFreeColor ( red );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST fill_rectangle_fills_all ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor blue = IAllocColor ( 0, 0, 255 );
  int x, y, all_blue = 1;

  ISetForeground ( gc, blue );
  ASSERT_EQ ( INoError, IFillRectangle ( im, gc, 0, 0, W, H ) );

  for ( y = 0; y < H; y++ )
    for ( x = 0; x < W; x++ )
      if ( px_r ( im, x, y ) != 0 || px_g ( im, x, y ) != 0 ||
           px_b ( im, x, y ) != 255 )
        all_blue = 0;
  ASSERT ( all_blue );

  IFreeColor ( blue );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST draw_line_marks_endpoints ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, IDrawLine ( im, gc, 0, 0, 9, 9 ) );

  /* both endpoints of the diagonal should be black */
  ASSERT_EQ ( 0, px_r ( im, 0, 0 ) );
  ASSERT_EQ ( 0, px_r ( im, 9, 9 ) );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST greyscale_image_is_single_channel ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_GREYSCALE );
  ASSERT ( px_grey ( im ) );
  /* freshly created image is white */
  ASSERT_EQ ( 255, px_r ( im, 0, 0 ) );
  IFreeImage ( im );
  PASS ();
}

SUITE ( draw )
{
  RUN_TEST ( draw_point_sets_one_pixel );
  RUN_TEST ( fill_rectangle_fills_all );
  RUN_TEST ( draw_line_marks_endpoints );
  RUN_TEST ( greyscale_image_is_single_channel );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( draw );
  GREATEST_MAIN_END ();
}
