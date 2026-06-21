/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ichart.c
 *
 * Ilib charting demo (installed-style example).
 *
 * Description:
 *	Builds a line, bar and pie chart with the Ichart API, then combines them
 *	into one image with IMontage and writes it out (default charts.png).
 *	The label font is compiled in from a BDF font, so the demo is
 *	self-contained.
 *
 *	Usage:  ilib-chart [outfile]
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <Ichart.h>
#include <Ilib.h>

/* Compiled-in font (generated from fonts/helvR08.bdf by ifont2h). */
#include "helvR08.h"

int main ( int argc, char *argv[] )
{
  const char *outfile = ( argc > 1 ) ? argv[1] : "charts.png";
  const char *months[5] = { "Jan", "Feb", "Mar", "Apr", "May" };
  const char *slices[3] = { "Direct", "Email", "Social" };
  double y2023[5] = { 3, 5, 4, 7, 6 };
  double y2024[5] = { 2, 3, 5, 4, 8 };
  double share[3] = { 45, 30, 25 };
  const char *quarters[4] = { "Q1", "Q2", "Q3", "Q4" };
  double area_y[6] = { 2, 4, 3, 6, 5, 7 };
  const char *days[6] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  double sx[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  double sy[8] = { 2.1, 3.4, 2.9, 5.1, 4.2, 6.3, 5.5, 7.8 };
  double prodA[4] = { 5, 7, 6, 9 };
  double prodB[4] = { 3, 4, 5, 4 };
  double prodC[4] = { 2, 3, 2, 5 };
  double growth[6] = { 2, 9, 40, 150, 700, 3000 };
  const char *gmonths[6] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun" };
  IFont font;
  IChart lc, bc, pc, ac, sc, gc_chart;
  IImage line_img, bar_img, pie_img, area_img, scatter_img, log_img, montage;
  IImage list[6];
  IFileFormat fmt = IFORMAT_PNG;
  FILE *fp;
  IError ret;

  if ( ILoadFontFromData ( "helv8", helvR08_font, &font ) != INoError ) {
    fprintf ( stderr, "Failed to load the built-in font.\n" );
    return ( 1 );
  }

  /* Line chart with two series. */
  lc = ICreateChart ( ICHART_LINE, 320, 240 );
  IChartSetFont ( lc, font );
  IChartSetTitle ( lc, "Monthly Sales" );
  IChartSetAxisLabels ( lc, "month", "k$" );
  IChartSetCategories ( lc, months, 5 );
  IChartAddSeries ( lc, "2023", y2023, 5, IAllocColor ( 0x4e, 0x79, 0xa7 ) );
  IChartAddSeries ( lc, "2024", y2024, 5, IAllocColor ( 0xe1, 0x57, 0x59 ) );
  line_img = IChartRender ( lc );

  /* Stacked bar chart. */
  bc = ICreateChart ( ICHART_BAR, 320, 240 );
  IChartSetFont ( bc, font );
  IChartSetStacked ( bc, 1 );
  IChartSetTitle ( bc, "Revenue by Product" );
  IChartSetCategories ( bc, quarters, 4 );
  IChartAddSeries ( bc, "A", prodA, 4, IAllocColor ( 0x4e, 0x79, 0xa7 ) );
  IChartAddSeries ( bc, "B", prodB, 4, IAllocColor ( 0xf2, 0x8e, 0x2b ) );
  IChartAddSeries ( bc, "C", prodC, 4, IAllocColor ( 0x59, 0xa1, 0x4f ) );
  bar_img = IChartRender ( bc );

  /* Pie chart. */
  pc = ICreateChart ( ICHART_PIE, 260, 240 );
  IChartSetFont ( pc, font );
  IChartSetTitle ( pc, "Traffic Sources" );
  IChartSetCategories ( pc, slices, 3 );
  IChartAddSeries ( pc, NULL, share, 3, IAllocColor ( 0, 0, 0 ) );
  pie_img = IChartRender ( pc );

  /* Area chart. */
  ac = ICreateChart ( ICHART_AREA, 320, 240 );
  IChartSetFont ( ac, font );
  IChartSetTitle ( ac, "Daily Visitors" );
  IChartSetCategories ( ac, days, 6 );
  IChartAddSeries ( ac, "visits", area_y, 6, IAllocColor ( 0x76, 0xb7, 0xb2 ) );
  area_img = IChartRender ( ac );

  /* Scatter chart. */
  sc = ICreateChart ( ICHART_SCATTER, 320, 240 );
  IChartSetFont ( sc, font );
  IChartSetTitle ( sc, "Height vs Weight" );
  IChartSetAxisLabels ( sc, "x", "y" );
  IChartAddXYSeries ( sc, "samples", sx, sy, 8,
    IAllocColor ( 0xb0, 0x7a, 0xa1 ) );
  scatter_img = IChartRender ( sc );

  /* Line chart on a logarithmic value axis. */
  gc_chart = ICreateChart ( ICHART_LINE, 320, 240 );
  IChartSetFont ( gc_chart, font );
  IChartSetLogScale ( gc_chart, 1 );
  IChartSetTitle ( gc_chart, "Growth (log scale)" );
  IChartSetCategories ( gc_chart, gmonths, 6 );
  IChartAddSeries ( gc_chart, "users", growth, 6,
    IAllocColor ( 0xe1, 0x57, 0x59 ) );
  log_img = IChartRender ( gc_chart );

  if ( !line_img || !bar_img || !pie_img || !area_img || !scatter_img ||
       !log_img ) {
    fprintf ( stderr, "Chart rendering failed.\n" );
    return ( 1 );
  }

  /* Combine all six charts into one image. */
  list[0] = line_img;
  list[1] = bar_img;
  list[2] = pie_img;
  list[3] = area_img;
  list[4] = scatter_img;
  list[5] = log_img;
  montage = IMontage ( list, 6, 3, 8, IAllocColor ( 245, 245, 245 ) );
  if ( !montage ) {
    fprintf ( stderr, "Montage failed.\n" );
    return ( 1 );
  }

  IFileType ( (char *) outfile, &fmt );
  fp = fopen ( outfile, "wb" );
  if ( !fp ) {
    perror ( "open output" );
    return ( 1 );
  }
  ret = IWriteImageFile ( fp, montage, fmt, IOPTION_NONE );
  fclose ( fp );
  if ( ret != INoError ) {
    fprintf ( stderr, "Write failed: %s\n", IErrorString ( ret ) );
    return ( 1 );
  }
  printf ( "Wrote %s\n", outfile );

  IFreeImage ( montage );
  IFreeImage ( line_img );
  IFreeImage ( bar_img );
  IFreeImage ( pie_img );
  IFreeImage ( area_img );
  IFreeImage ( scatter_img );
  IFreeImage ( log_img );
  IFreeChart ( lc );
  IFreeChart ( bc );
  IFreeChart ( pc );
  IFreeChart ( ac );
  IFreeChart ( sc );
  IFreeChart ( gc_chart );
  IFreeFont ( font );
  return ( 0 );
}
