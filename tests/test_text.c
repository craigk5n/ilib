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

SUITE ( text )
{
  RUN_TEST ( load_font_succeeds );
  RUN_TEST ( load_missing_font_fails );
  RUN_TEST ( text_dimensions_positive );
  RUN_TEST ( draw_string_succeeds );
  RUN_TEST ( ttf_text_is_antialiased );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( text );
  GREATEST_MAIN_END ();
}
