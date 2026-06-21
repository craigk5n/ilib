/* SPDX-License-Identifier: GPL-2.0-only */
/* Charting (IChart.c). Rendered without a font (text is skipped), so these
   check structure: a chart image of the right size with data drawn on it. */

#include <Ichart.h>
#include <Ilib.h>
#include "greatest.h"

/* Is there any non-white pixel (i.e. something was drawn)? */
static int has_drawn_pixel ( IImage im )
{
  int w = (int) IImageWidth ( im ), h = (int) IImageHeight ( im ), x, y;
  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < w; x++ ) {
      unsigned int r, g, b;
      IGetPixel ( im, x, y, &r, &g, &b );
      if ( !( r == 255 && g == 255 && b == 255 ) )
        return ( 1 );
    }
  }
  return ( 0 );
}

TEST line_chart_renders ( void )
{
  IChart c = ICreateChart ( ICHART_LINE, 200, 150 );
  double v[5] = { 1.0, 3.0, 2.0, 5.0, 4.0 };
  IImage img;

  ASSERT ( c != NULL );
  ASSERT_EQ ( INoError, IChartAddSeries ( c, "a", v, 5, IAllocColor ( 200, 0, 0 ) ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT_EQ ( 200, (int) IImageWidth ( img ) );
  ASSERT_EQ ( 150, (int) IImageHeight ( img ) );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST bar_chart_renders ( void )
{
  IChart c = ICreateChart ( ICHART_BAR, 220, 160 );
  double a[4] = { 2.0, 4.0, 3.0, 6.0 };
  double b[4] = { 1.0, 2.0, 5.0, 2.0 };
  IImage img;

  ASSERT ( c != NULL );
  IChartAddSeries ( c, "a", a, 4, IAllocColor ( 50, 100, 200 ) );
  IChartAddSeries ( c, "b", b, 4, IAllocColor ( 200, 100, 50 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT_EQ ( 220, (int) IImageWidth ( img ) );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST pie_chart_renders ( void )
{
  IChart c = ICreateChart ( ICHART_PIE, 160, 160 );
  double v[3] = { 30.0, 50.0, 20.0 };
  const char *cats[3] = { "x", "y", "z" };
  IImage img;

  ASSERT ( c != NULL );
  IChartSetCategories ( c, cats, 3 );
  IChartAddSeries ( c, NULL, v, 3, IAllocColor ( 0, 0, 0 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT_EQ ( 160, (int) IImageWidth ( img ) );
  /* The pie's centre should be a coloured (non-white) slice. */
  {
    unsigned int r, g, b;
    IGetPixel ( img, 80, 80, &r, &g, &b );
    ASSERT ( !( r == 255 && g == 255 && b == 255 ) );
  }

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST area_chart_renders ( void )
{
  IChart c = ICreateChart ( ICHART_AREA, 200, 150 );
  double v[5] = { 1.0, 3.0, 2.0, 5.0, 4.0 };
  IImage img;

  ASSERT ( c != NULL );
  IChartAddSeries ( c, "a", v, 5, IAllocColor ( 80, 140, 200 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT_EQ ( 200, (int) IImageWidth ( img ) );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST scatter_chart_renders ( void )
{
  IChart c = ICreateChart ( ICHART_SCATTER, 200, 150 );
  double x[4] = { 1.0, 4.0, 2.0, 8.0 };
  double y[4] = { 2.0, 5.0, 3.0, 7.0 };
  IImage img;

  ASSERT ( c != NULL );
  ASSERT_EQ ( INoError,
    IChartAddXYSeries ( c, "pts", x, y, 4, IAllocColor ( 200, 0, 0 ) ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT_EQ ( 200, (int) IImageWidth ( img ) );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST add_xy_series_rejects_bad_args ( void )
{
  IChart c = ICreateChart ( ICHART_SCATTER, 100, 100 );
  double x[2] = { 1.0, 2.0 }, y[2] = { 1.0, 2.0 };

  ASSERT_EQ ( IInvalidChart, IChartAddXYSeries ( NULL, "a", x, y, 2, 0 ) );
  ASSERT_EQ ( IInvalidArgument, IChartAddXYSeries ( c, "a", NULL, y, 2, 0 ) );
  ASSERT_EQ ( IInvalidArgument, IChartAddXYSeries ( c, "a", x, y, 0, 0 ) );

  IFreeChart ( c );
  PASS ();
}

TEST stacked_bar_renders ( void )
{
  IChart c = ICreateChart ( ICHART_BAR, 220, 160 );
  double a[3] = { 2.0, 4.0, 3.0 };
  double b[3] = { 1.0, 2.0, 5.0 };
  IImage img;

  ASSERT ( c != NULL );
  ASSERT_EQ ( INoError, IChartSetStacked ( c, 1 ) );
  IChartAddSeries ( c, "a", a, 3, IAllocColor ( 50, 100, 200 ) );
  IChartAddSeries ( c, "b", b, 3, IAllocColor ( 200, 100, 50 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST log_scale_renders ( void )
{
  IChart c = ICreateChart ( ICHART_LINE, 220, 160 );
  double v[4] = { 1.0, 10.0, 100.0, 1000.0 };
  IImage img;

  ASSERT ( c != NULL );
  ASSERT_EQ ( INoError, IChartSetLogScale ( c, 1 ) );
  IChartAddSeries ( c, "exp", v, 4, IAllocColor ( 0, 0, 0 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST value_labels_render ( void )
{
  IChart c = ICreateChart ( ICHART_BAR, 220, 160 );
  double v[3] = { 2.0, 4.0, 3.0 };
  IImage img;

  /* Without a font, value labels are simply skipped (no crash). */
  ASSERT_EQ ( INoError, IChartSetValueLabels ( c, 1 ) );
  IChartAddSeries ( c, "a", v, 3, IAllocColor ( 50, 100, 200 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );
  ASSERT ( has_drawn_pixel ( img ) );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST chart_setters_reject_bad_handle ( void )
{
  ASSERT_EQ ( IInvalidChart, IChartSetStacked ( NULL, 1 ) );
  ASSERT_EQ ( IInvalidChart, IChartSetLogScale ( NULL, 1 ) );
  ASSERT_EQ ( IInvalidChart, IChartSetValueLabels ( NULL, 1 ) );
  PASS ();
}

TEST chart_explicit_range ( void )
{
  IChart c = ICreateChart ( ICHART_LINE, 120, 100 );
  double v[3] = { 1.0, 2.0, 3.0 };
  IImage img;

  ASSERT_EQ ( INoError, IChartSetRange ( c, 0.0, 10.0 ) );
  ASSERT_EQ ( IInvalidArgument, IChartSetRange ( c, 5.0, 5.0 ) );
  IChartAddSeries ( c, "a", v, 3, IAllocColor ( 0, 0, 0 ) );
  img = IChartRender ( c );
  ASSERT ( img != NULL );

  IFreeImage ( img );
  IFreeChart ( c );
  PASS ();
}

TEST chart_setters_and_titles ( void )
{
  IChart c = ICreateChart ( ICHART_LINE, 100, 100 );
  double v[2] = { 1.0, 2.0 };

  ASSERT_EQ ( INoError, IChartSetTitle ( c, "T" ) );
  ASSERT_EQ ( INoError, IChartSetAxisLabels ( c, "x", "y" ) );
  ASSERT_EQ ( INoError, IChartSetBackground ( c, IAllocColor ( 240, 240, 240 ) ) );
  ASSERT_EQ ( INoError, IChartSetFont ( c, NULL ) );
  IChartAddSeries ( c, "a", v, 2, IAllocColor ( 0, 0, 0 ) );
  /* A grey background means every pixel is non-white; just ensure it renders. */
  {
    IImage img = IChartRender ( c );
    ASSERT ( img != NULL );
    IFreeImage ( img );
  }

  IFreeChart ( c );
  PASS ();
}

TEST chart_rejects_bad_args ( void )
{
  IChart c = ICreateChart ( ICHART_LINE, 100, 100 );
  double v[2] = { 1.0, 2.0 };

  ASSERT ( ICreateChart ( ICHART_LINE, 0, 100 ) == NULL );
  ASSERT_EQ ( IInvalidChart, IChartSetTitle ( NULL, "x" ) );
  ASSERT_EQ ( IInvalidChart, IChartAddSeries ( NULL, "a", v, 2, 0 ) );
  ASSERT_EQ ( IInvalidArgument, IChartAddSeries ( c, "a", v, 0, 0 ) );
  ASSERT ( IChartRender ( NULL ) == NULL );
  ASSERT_EQ ( IInvalidChart, _IFreeChart ( NULL ) );

  IFreeChart ( c );
  PASS ();
}

SUITE ( chart )
{
  RUN_TEST ( line_chart_renders );
  RUN_TEST ( bar_chart_renders );
  RUN_TEST ( pie_chart_renders );
  RUN_TEST ( area_chart_renders );
  RUN_TEST ( scatter_chart_renders );
  RUN_TEST ( add_xy_series_rejects_bad_args );
  RUN_TEST ( stacked_bar_renders );
  RUN_TEST ( log_scale_renders );
  RUN_TEST ( value_labels_render );
  RUN_TEST ( chart_setters_reject_bad_handle );
  RUN_TEST ( chart_explicit_range );
  RUN_TEST ( chart_setters_and_titles );
  RUN_TEST ( chart_rejects_bad_args );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( chart );
  GREATEST_MAIN_END ();
}
