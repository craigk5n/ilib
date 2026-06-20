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

/* True if any pixel is a partial (blended) grey -- the signature of AA. */
static int has_blended_pixel ( IImage im )
{
  int x, y, v;
  for ( y = 0; y < H; y++ ) {
    for ( x = 0; x < W; x++ ) {
      v = px_r ( im, x, y );
      if ( v > 0 && v < 255 )
        return ( 1 );
    }
  }
  return ( 0 );
}

/* Without anti-aliasing a black line on white leaves only pure black/white. */
TEST aliased_line_is_binary ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  IDrawLine ( im, gc, 0, 0, 9, 3 ); /* shallow diagonal */
  ASSERT_FALSE ( has_blended_pixel ( im ) );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* With anti-aliasing the same line produces blended edge pixels, and the
   endpoints are still covered. */
TEST aa_line_blends_edges ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, ISetAntiAlias ( gc, 1 ) );
  IDrawLine ( im, gc, 0, 0, 9, 3 );

  ASSERT ( has_blended_pixel ( im ) );
  /* endpoints are (close to) fully covered */
  ASSERT ( px_r ( im, 0, 0 ) < 128 );
  ASSERT ( px_r ( im, 9, 3 ) < 128 );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST set_antialias_rejects_bad_handle ( void )
{
  unsigned int not_a_gc = 0;
  ASSERT_EQ ( IInvalidGC, ISetAntiAlias ( NULL, 1 ) );
  ASSERT_EQ ( IInvalidGC, ISetAntiAlias ( (IGC) &not_a_gc, 1 ) );
  PASS ();
}

/* An anti-aliased filled circle (IFillCircle -> IFillPolygon) has a solid
   interior and blended edge pixels, and leaves the corners untouched. */
TEST aa_fill_circle_blends_edges ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, ISetAntiAlias ( gc, 1 ) );
  ASSERT_EQ ( INoError, IFillCircle ( im, gc, 5, 5, 3 ) );

  ASSERT_EQ ( 0, px_r ( im, 5, 5 ) );   /* interior fully covered */
  ASSERT_EQ ( 255, px_r ( im, 0, 0 ) ); /* corner untouched */
  ASSERT ( has_blended_pixel ( im ) );  /* soft edge */

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* An anti-aliased filled triangle has interior coverage and soft edges. */
TEST aa_fill_polygon_blends_edges ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );
  IPoint tri[3] = { { 1, 1 }, { 8, 2 }, { 3, 8 } };

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, ISetAntiAlias ( gc, 1 ) );
  ASSERT_EQ ( INoError, IFillPolygon ( im, gc, tri, 3 ) );

  ASSERT ( px_r ( im, 3, 3 ) < 128 ); /* well inside the triangle */
  ASSERT ( has_blended_pixel ( im ) );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* An anti-aliased ellipse outline blends edge pixels, covers a point on the
   curve, and leaves the interior (it is an outline) untouched. */
TEST aa_ellipse_outline_blends_edges ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, ISetAntiAlias ( gc, 1 ) );
  ASSERT_EQ ( INoError, IDrawEllipse ( im, gc, 5, 5, 4, 2 ) );

  ASSERT ( px_r ( im, 9, 5 ) < 128 );   /* rightmost point on the curve */
  ASSERT_EQ ( 255, px_r ( im, 5, 5 ) ); /* interior untouched (outline) */
  ASSERT ( has_blended_pixel ( im ) );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* An anti-aliased filled ellipse has a solid interior and soft edges. */
TEST aa_fill_ellipse_blends_edges ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, ISetAntiAlias ( gc, 1 ) );
  ASSERT_EQ ( INoError, IFillEllipse ( im, gc, 5, 5, 4, 2 ) );

  ASSERT_EQ ( 0, px_r ( im, 5, 5 ) );   /* interior filled */
  ASSERT_EQ ( 255, px_r ( im, 0, 0 ) ); /* corner untouched */
  ASSERT ( has_blended_pixel ( im ) );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* An anti-aliased circle covers its cardinal points and produces blended
   edge pixels. */
TEST aa_circle_blends_edges ( void )
{
  IImage im = ICreateImage ( W, H, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor black = IAllocColor ( 0, 0, 0 );

  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, ISetAntiAlias ( gc, 1 ) );
  ASSERT_EQ ( INoError, IDrawCircle ( im, gc, 5, 5, 3 ) );

  /* cardinal points of the radius-3 circle are fully covered (dark) */
  ASSERT ( px_r ( im, 8, 5 ) < 128 );
  ASSERT ( px_r ( im, 2, 5 ) < 128 );
  ASSERT ( px_r ( im, 5, 8 ) < 128 );
  ASSERT ( px_r ( im, 5, 2 ) < 128 );
  /* the center is untouched (white) */
  ASSERT_EQ ( 255, px_r ( im, 5, 5 ) );
  /* and there is at least one blended edge pixel */
  ASSERT ( has_blended_pixel ( im ) );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

SUITE ( draw )
{
  RUN_TEST ( draw_point_sets_one_pixel );
  RUN_TEST ( fill_rectangle_fills_all );
  RUN_TEST ( draw_line_marks_endpoints );
  RUN_TEST ( greyscale_image_is_single_channel );
  RUN_TEST ( aliased_line_is_binary );
  RUN_TEST ( aa_line_blends_edges );
  RUN_TEST ( set_antialias_rejects_bad_handle );
  RUN_TEST ( aa_circle_blends_edges );
  RUN_TEST ( aa_fill_circle_blends_edges );
  RUN_TEST ( aa_fill_polygon_blends_edges );
  RUN_TEST ( aa_ellipse_outline_blends_edges );
  RUN_TEST ( aa_fill_ellipse_blends_edges );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( draw );
  GREATEST_MAIN_END ();
}
