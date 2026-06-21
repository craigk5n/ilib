/* SPDX-License-Identifier: GPL-2.0-only */
/* X11 BDF font loading and text measurement/drawing.
   ILIB_TEST_FONT_DIR is defined by CMake to the source fonts/ directory. */

#include <stdio.h>
#include <string.h>
#include <Ilib.h>
#include "greatest.h"

#ifndef ILIB_TEST_FONT_DIR
#define ILIB_TEST_FONT_DIR "fonts"
#endif

#define FONT_PATH ILIB_TEST_FONT_DIR "/helvR08.bdf"

TEST load_font_succeeds ( void )
{
  IFont font = NULL;
  ASSERT_EQ ( INoError,
    ILoadFontFromFile ( "helvR08", FONT_PATH, &font ) );
  ASSERT ( font != NULL );
  IFreeFont ( font );
  PASS ();
}

TEST load_missing_font_fails ( void )
{
  IFont font = NULL;
  ASSERT ( ILoadFontFromFile ( "nope",
             ILIB_TEST_FONT_DIR "/does_not_exist.bdf", &font ) != INoError );
  PASS ();
}

TEST text_dimensions_positive ( void )
{
  IGC gc = ICreateGC ();
  IFont font = NULL;
  unsigned int w = 0, h = 0;
  const char *s = "Hello";

  ASSERT_EQ ( INoError, ILoadFontFromFile ( "helvR08", FONT_PATH, &font ) );
  ISetFont ( gc, font );

  ASSERT_EQ ( INoError, ITextWidth ( gc, font, (char *) s, strlen ( s ), &w ) );
  ASSERT_EQ ( INoError, ITextHeight ( gc, font, (char *) s, strlen ( s ), &h ) );
  ASSERT ( w > 0 );
  ASSERT ( h > 0 );

  IFreeFont ( font );
  IFreeGC ( gc );
  PASS ();
}

TEST draw_string_succeeds ( void )
{
  IImage im = ICreateImage ( 80, 20, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IFont font = NULL;
  IColor black = IAllocColor ( 0, 0, 0 );
  const char *s = "Hi";

  ASSERT_EQ ( INoError, ILoadFontFromFile ( "helvR08", FONT_PATH, &font ) );
  ISetFont ( gc, font );
  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError,
    IDrawString ( im, gc, 2, 15, (char *) s, strlen ( s ) ) );

  IFreeColor ( black );
  IFreeFont ( font );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST text_alignment_coordinates ( void )
{
  IGC gc = ICreateGC ();
  IFont font = NULL;
  unsigned int w = 0, h = 0;
  int x = -1, y = -1;
  const char *s = "Hello";

  ASSERT_EQ ( INoError, ILoadFontFromFile ( "helvR08", FONT_PATH, &font ) );
  ITextDimensions ( gc, font, (char *) s, strlen ( s ), &w, &h );

  /* Left/bottom anchor == the raw draw position. */
  ASSERT_EQ ( INoError,
    ICalculateTextCoordinates ( gc, font, (char *) s, strlen ( s ), 100, 100,
      IALIGN_LEFT, IALIGN_BOTTOM, &x, &y ) );
  ASSERT_EQ ( 100, x );
  ASSERT_EQ ( 100, y );

  /* Centred horizontally, middle vertically. */
  ICalculateTextCoordinates ( gc, font, (char *) s, strlen ( s ), 100, 100,
    IALIGN_CENTER, IALIGN_MIDDLE, &x, &y );
  ASSERT_EQ ( 100 - (int) w / 2, x );
  ASSERT_EQ ( 100 + (int) h / 2, y );

  /* Right-aligned, top anchor. */
  ICalculateTextCoordinates ( gc, font, (char *) s, strlen ( s ), 100, 100,
    IALIGN_RIGHT, IALIGN_TOP, &x, &y );
  ASSERT_EQ ( 100 - (int) w, x );
  ASSERT_EQ ( 100 + (int) h, y );

  IFreeFont ( font );
  IFreeGC ( gc );
  PASS ();
}

/* Scalable TrueType text via FreeType. Skipped cleanly when the library was
   built without FreeType, or when no system TrueType font can be found. */
TEST ttf_text_is_antialiased ( void )
{
  static const char *candidates[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
    "/Library/Fonts/Arial.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf", NULL };
  IFont font = NULL;
  IError ret;
  const char *path = NULL;
  int i, x, y, dark = 0, blended = 0;
  unsigned int r;
  IImage im;
  IGC gc;
  IColor black;

  /* A bogus path tells us whether FreeType is compiled in at all. */
  ret = ILoadFontFromFileTTF ( "probe", "/no/such/font.ttf", 16, &font );
  if ( ret == IFunctionNotImplemented )
    SKIPm ( "built without FreeType" );
  ASSERT ( ret != INoError ); /* a missing file must not succeed */

  for ( i = 0; candidates[i]; i++ ) {
    FILE *f = fopen ( candidates[i], "rb" );
    if ( f ) {
      fclose ( f );
      path = candidates[i];
      break;
    }
  }
  if ( !path )
    SKIPm ( "no system TrueType font available" );

  ASSERT_EQ ( INoError, ILoadFontFromFileTTF ( "tt", (char *) path, 24, &font ) );
  im = ICreateImage ( 120, 40, IOPTION_NONE );
  gc = ICreateGC ();
  black = IAllocColor ( 0, 0, 0 );
  ISetFont ( gc, font );
  ISetForeground ( gc, black );
  ASSERT_EQ ( INoError, IDrawString ( im, gc, 5, 30, "Ag", 2 ) );

  for ( y = 0; y < 40; y++ ) {
    for ( x = 0; x < 120; x++ ) {
      IGetPixel ( im, x, y, &r, NULL, NULL );
      if ( r < 40 )
        dark = 1; /* glyph ink */
      if ( r > 20 && r < 235 )
        blended = 1; /* anti-aliased edge */
    }
  }
  ASSERT ( dark );
  ASSERT ( blended );

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( im );
  IFreeFont ( font );
  PASS ();
}

/* Return a usable system TrueType font path, or NULL if none is found. */
static const char *ttf_find_path ( void )
{
  static const char *cands[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf", "/Library/Fonts/Arial.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf", NULL };
  int i;
  for ( i = 0; cands[i]; i++ ) {
    FILE *f = fopen ( cands[i], "rb" );
    if ( f ) {
      fclose ( f );
      return cands[i];
    }
  }
  return NULL;
}

/* Rotated TTF text occupies a tall/narrow region where horizontal text is
   wide/short -- i.e. rotation is actually applied. */
TEST ttf_text_rotation ( void )
{
  IFont font = NULL;
  const char *path;
  IImage h, v;
  IGC gc;
  IColor black;
  int x, y, hw, hh, vw, vh;
  int hx0 = 99, hx1 = -1, hy0 = 99, hy1 = -1;
  int vx0 = 99, vx1 = -1, vy0 = 99, vy1 = -1;
  unsigned int r;

  if ( ILoadFontFromFileTTF ( "p", "/no/such.ttf", 16, &font ) == IFunctionNotImplemented )
    SKIPm ( "built without FreeType" );
  path = ttf_find_path ();
  if ( !path )
    SKIPm ( "no system TrueType font available" );

  ASSERT_EQ ( INoError, ILoadFontFromFileTTF ( "rt", (char *) path, 18, &font ) );
  gc = ICreateGC ();
  black = IAllocColor ( 0, 0, 0 );
  ISetFont ( gc, font );
  ISetForeground ( gc, black );

  h = ICreateImage ( 60, 60, IOPTION_NONE );
  v = ICreateImage ( 60, 60, IOPTION_NONE );
  ASSERT_EQ ( INoError, IDrawString ( h, gc, 5, 30, "Hi", 2 ) );
  ASSERT_EQ ( INoError,
    IDrawStringRotated ( v, gc, 30, 50, "Hi", 2, ITEXT_BOTTOM_TO_TOP ) );

  for ( y = 0; y < 60; y++ ) {
    for ( x = 0; x < 60; x++ ) {
      IGetPixel ( h, x, y, &r, NULL, NULL );
      if ( r < 200 ) {
        if ( x < hx0 )
          hx0 = x;
        if ( x > hx1 )
          hx1 = x;
        if ( y < hy0 )
          hy0 = y;
        if ( y > hy1 )
          hy1 = y;
      }
      IGetPixel ( v, x, y, &r, NULL, NULL );
      if ( r < 200 ) {
        if ( x < vx0 )
          vx0 = x;
        if ( x > vx1 )
          vx1 = x;
        if ( y < vy0 )
          vy0 = y;
        if ( y > vy1 )
          vy1 = y;
      }
    }
  }
  hw = hx1 - hx0;
  hh = hy1 - hy0;
  vw = vx1 - vx0;
  vh = vy1 - vy0;
  ASSERT ( hw > hh ); /* horizontal text is wider than tall */
  ASSERT ( vh > vw ); /* rotated text is taller than wide */

  IFreeColor ( black );
  IFreeGC ( gc );
  IFreeImage ( h );
  IFreeImage ( v );
  IFreeFont ( font );
  PASS ();
}

/* A text style (shadowed) draws the glyphs several times, so it marks strictly
   more pixels than plain text -- confirming styles apply to TTF too. */
TEST ttf_text_style_adds_ink ( void )
{
  IFont font = NULL;
  const char *path;
  IImage plain, styled;
  IGC gc;
  IColor black, grey;
  int x, y, np = 0, ns = 0;
  unsigned int r;

  if ( ILoadFontFromFileTTF ( "p", "/no/such.ttf", 16, &font ) == IFunctionNotImplemented )
    SKIPm ( "built without FreeType" );
  path = ttf_find_path ();
  if ( !path )
    SKIPm ( "no system TrueType font available" );

  ASSERT_EQ ( INoError, ILoadFontFromFileTTF ( "st", (char *) path, 18, &font ) );
  gc = ICreateGC ();
  black = IAllocColor ( 0, 0, 0 );
  grey = IAllocColor ( 128, 128, 128 );
  ISetFont ( gc, font );
  ISetForeground ( gc, black );
  ISetBackground ( gc, grey );

  plain = ICreateImage ( 80, 40, IOPTION_NONE );
  styled = ICreateImage ( 80, 40, IOPTION_NONE );
  ISetTextStyle ( gc, ITEXT_NORMAL );
  ASSERT_EQ ( INoError, IDrawString ( plain, gc, 5, 28, "Hi", 2 ) );
  ISetTextStyle ( gc, ITEXT_SHADOWED );
  ASSERT_EQ ( INoError, IDrawString ( styled, gc, 5, 28, "Hi", 2 ) );

  for ( y = 0; y < 40; y++ ) {
    for ( x = 0; x < 80; x++ ) {
      IGetPixel ( plain, x, y, &r, NULL, NULL );
      if ( r < 250 )
        np++;
      IGetPixel ( styled, x, y, &r, NULL, NULL );
      if ( r < 250 )
        ns++;
    }
  }
  ASSERT ( np > 0 );
  ASSERT ( ns > np ); /* the shadow adds pixels */

  IFreeColor ( black );
  IFreeColor ( grey );
  IFreeGC ( gc );
  IFreeImage ( plain );
  IFreeImage ( styled );
  IFreeFont ( font );
  PASS ();
}

/* Regression: ITextWidth must report a real width for scalable (TTF) fonts.
   It previously always used the BDF glyph cache and returned ~0 for TTF. */
TEST ttf_text_width_scales ( void )
{
  IGC gc;
  IFont font;
  const char *path;
  unsigned int w1 = 0, w2 = 0, h = 0;

  if ( ILoadFontFromFileTTF ( "p", "/no/such.ttf", 16, &font ) ==
       IFunctionNotImplemented )
    SKIPm ( "built without FreeType" );
  path = ttf_find_path ();
  if ( !path )
    SKIPm ( "no system TrueType font available" );

  ASSERT_EQ ( INoError, ILoadFontFromFileTTF ( "wt", (char *) path, 20, &font ) );
  gc = ICreateGC ();
  ASSERT_EQ ( INoError, ITextWidth ( gc, font, (char *) "i", 1, &w1 ) );
  ASSERT_EQ ( INoError, ITextWidth ( gc, font, (char *) "wide text", 9, &w2 ) );
  ASSERT_EQ ( INoError, ITextHeight ( gc, font, (char *) "x", 1, &h ) );
  ASSERT ( w1 > 0 );  /* the bug returned 0 here */
  ASSERT ( w2 > w1 ); /* longer text is wider */
  ASSERT ( h > 0 );

  IFreeGC ( gc );
  IFreeFont ( font );
  PASS ();
}

SUITE ( text )
{
  RUN_TEST ( load_font_succeeds );
  RUN_TEST ( load_missing_font_fails );
  RUN_TEST ( text_dimensions_positive );
  RUN_TEST ( text_alignment_coordinates );
  RUN_TEST ( ttf_text_width_scales );
  RUN_TEST ( draw_string_succeeds );
  RUN_TEST ( ttf_text_is_antialiased );
  RUN_TEST ( ttf_text_rotation );
  RUN_TEST ( ttf_text_style_adds_ink );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( text );
  GREATEST_MAIN_END ();
}
