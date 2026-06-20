/* SPDX-License-Identifier: GPL-2.0-only */
/* Public per-pixel get/set accessors. These are self-verifying (set then get)
   so no white-box access is needed. */

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

SUITE ( pixel )
{
  RUN_TEST ( set_then_get_roundtrips );
  RUN_TEST ( get_allows_null_channels );
  RUN_TEST ( greyscale_uses_single_value );
  RUN_TEST ( out_of_bounds_rejected );
  RUN_TEST ( channel_over_255_rejected );
  RUN_TEST ( bad_handle_rejected );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( pixel );
  GREATEST_MAIN_END ();
}
