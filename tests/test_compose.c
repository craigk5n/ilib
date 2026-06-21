/* SPDX-License-Identifier: GPL-2.0-only */
/* Composition operations (ICompose.c): trim, border, append, montage. */

#include <Ilib.h>
#include "greatest.h"

static unsigned int red_at ( IImage im, int x, int y )
{
  unsigned int r = 0;
  IGetPixel ( im, x, y, &r, NULL, NULL );
  return ( r );
}

static void fill ( IImage im, int w, int h, unsigned int r, unsigned int g,
  unsigned int b )
{
  int x, y;
  for ( y = 0; y < h; y++ )
    for ( x = 0; x < w; x++ )
      ISetPixel ( im, x, y, r, g, b );
}

TEST trim_crops_to_content ( void )
{
  IImage im = ICreateImage ( 10, 10, IOPTION_NONE ); /* white background */
  int x, y;

  for ( y = 3; y <= 6; y++ ) /* a 4x4 red block at (3,3) */
    for ( x = 3; x <= 6; x++ )
      ISetPixel ( im, x, y, 255, 0, 0 );

  ASSERT_EQ ( INoError, ITrim ( im, 0 ) );
  ASSERT_EQ ( 4, (int) IImageWidth ( im ) );
  ASSERT_EQ ( 4, (int) IImageHeight ( im ) );
  ASSERT_EQ ( 255, red_at ( im, 0, 0 ) ); /* top-left is now the red block */

  IFreeImage ( im );
  PASS ();
}

TEST trim_all_background_is_noop ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_NONE ); /* all white */

  ASSERT_EQ ( INoError, ITrim ( im, 0 ) );
  ASSERT_EQ ( 5, (int) IImageWidth ( im ) ); /* unchanged, not 0-sized */
  ASSERT_EQ ( 5, (int) IImageHeight ( im ) );

  IFreeImage ( im );
  PASS ();
}

TEST border_grows_and_frames ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  IColor black = IAllocColor ( 0, 0, 0 );

  fill ( im, 4, 4, 200, 0, 0 );
  ASSERT_EQ ( INoError, IBorder ( im, 2, black ) );
  ASSERT_EQ ( 8, (int) IImageWidth ( im ) ); /* +2 on each side */
  ASSERT_EQ ( 8, (int) IImageHeight ( im ) );
  ASSERT_EQ ( 0, red_at ( im, 0, 0 ) );   /* frame is black */
  ASSERT_EQ ( 200, red_at ( im, 2, 2 ) ); /* original at (2,2) */

  IFreeImage ( im );
  PASS ();
}

TEST border_rejects_bad_color ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );

  ASSERT_EQ ( IInvalidColor, IBorder ( im, 1, 999999u ) );

  IFreeImage ( im );
  PASS ();
}

TEST append_horizontal ( void )
{
  IImage a = ICreateImage ( 3, 2, IOPTION_NONE );
  IImage b = ICreateImage ( 2, 4, IOPTION_NONE );
  IColor green = IAllocColor ( 0, 255, 0 );
  IImage out;
  IImage list[2];

  fill ( a, 3, 2, 255, 0, 0 ); /* red */
  fill ( b, 2, 4, 0, 0, 255 ); /* blue */
  list[0] = a;
  list[1] = b;

  out = IAppend ( list, 2, 1, green );
  ASSERT ( out != NULL );
  ASSERT_EQ ( 5, (int) IImageWidth ( out ) );  /* 3 + 2 */
  ASSERT_EQ ( 4, (int) IImageHeight ( out ) ); /* max(2,4) */
  ASSERT_EQ ( 255, red_at ( out, 0, 0 ) );     /* a (red) */
  {
    unsigned int r, g, bl;
    IGetPixel ( out, 3, 0, &r, &g, &bl ); /* b (blue) at x=3 */
    ASSERT_EQ ( 0, r );
    ASSERT_EQ ( 255, bl );
    IGetPixel ( out, 0, 3, &r, &g, &bl ); /* gap below a -> green */
    ASSERT_EQ ( 0, r );
    ASSERT_EQ ( 255, g );
  }

  IFreeImage ( out );
  IFreeImage ( a );
  IFreeImage ( b );
  PASS ();
}

TEST append_vertical ( void )
{
  IImage a = ICreateImage ( 4, 2, IOPTION_NONE );
  IImage b = ICreateImage ( 2, 3, IOPTION_NONE );
  IColor green = IAllocColor ( 0, 255, 0 );
  IImage list[2], out;

  fill ( a, 4, 2, 255, 0, 0 );
  fill ( b, 2, 3, 0, 0, 255 );
  list[0] = a;
  list[1] = b;

  out = IAppend ( list, 2, 0, green );
  ASSERT ( out != NULL );
  ASSERT_EQ ( 4, (int) IImageWidth ( out ) );  /* max(4,2) */
  ASSERT_EQ ( 5, (int) IImageHeight ( out ) ); /* 2 + 3 */
  ASSERT_EQ ( 255, red_at ( out, 0, 0 ) );     /* a */

  IFreeImage ( out );
  IFreeImage ( a );
  IFreeImage ( b );
  PASS ();
}

TEST montage_grid ( void )
{
  IImage a = ICreateImage ( 2, 2, IOPTION_NONE );
  IImage b = ICreateImage ( 2, 2, IOPTION_NONE );
  IImage c = ICreateImage ( 2, 2, IOPTION_NONE );
  IColor green = IAllocColor ( 0, 255, 0 );
  IImage list[3], out;

  fill ( a, 2, 2, 255, 0, 0 );
  fill ( b, 2, 2, 255, 0, 0 );
  fill ( c, 2, 2, 255, 0, 0 );
  list[0] = a;
  list[1] = b;
  list[2] = c;

  /* 3 images, 2 columns, spacing 1, cells 2x2 -> rows 2.
     nw = 2*2 + 3*1 = 7; nh = 2*2 + 3*1 = 7. */
  out = IMontage ( list, 3, 2, 1, green );
  ASSERT ( out != NULL );
  ASSERT_EQ ( 7, (int) IImageWidth ( out ) );
  ASSERT_EQ ( 7, (int) IImageHeight ( out ) );
  ASSERT_EQ ( 0, red_at ( out, 0, 0 ) );   /* spacing border -> green */
  ASSERT_EQ ( 255, red_at ( out, 1, 1 ) ); /* first cell content -> red */

  IFreeImage ( out );
  IFreeImage ( a );
  IFreeImage ( b );
  IFreeImage ( c );
  PASS ();
}

TEST compose_rejects_bad_args ( void )
{
  IImage a = ICreateImage ( 2, 2, IOPTION_NONE );
  IColor green = IAllocColor ( 0, 255, 0 );
  IImage list[1];
  list[0] = a;

  ASSERT ( IAppend ( NULL, 2, 1, green ) == NULL );
  ASSERT ( IAppend ( list, 0, 1, green ) == NULL );
  ASSERT ( IAppend ( list, 1, 1, 999999u ) == NULL );    /* bad bg */
  ASSERT ( IMontage ( list, 1, 0, 0, green ) == NULL );  /* 0 columns */
  ASSERT ( IMontage ( list, 1, 1, -1, green ) == NULL ); /* negative spacing */

  IFreeImage ( a );
  PASS ();
}

TEST compose_rejects_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidImage, ITrim ( NULL, 0 ) );
  ASSERT_EQ ( IInvalidImage, IBorder ( NULL, 1, 0 ) );
  PASS ();
}

SUITE ( compose )
{
  RUN_TEST ( trim_crops_to_content );
  RUN_TEST ( trim_all_background_is_noop );
  RUN_TEST ( border_grows_and_frames );
  RUN_TEST ( border_rejects_bad_color );
  RUN_TEST ( append_horizontal );
  RUN_TEST ( append_vertical );
  RUN_TEST ( montage_grid );
  RUN_TEST ( compose_rejects_bad_args );
  RUN_TEST ( compose_rejects_bad_handle );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( compose );
  GREATEST_MAIN_END ();
}
