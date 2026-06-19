/* X11 BDF font loading and text measurement/drawing.
   ILIB_TEST_FONT_DIR is defined by CMake to the source fonts/ directory. */

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

SUITE ( text )
{
  RUN_TEST ( load_font_succeeds );
  RUN_TEST ( load_missing_font_fails );
  RUN_TEST ( text_dimensions_positive );
  RUN_TEST ( draw_string_succeeds );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( text );
  GREATEST_MAIN_END ();
}
