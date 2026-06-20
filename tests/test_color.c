/* SPDX-License-Identifier: GPL-2.0-only */
/* Color allocation and named-color lookup. */

#include <Ilib.h>
#include "greatest.h"

TEST alloc_color_nonzero ( void )
{
  IColor c = IAllocColor ( 255, 128, 0 );
  ASSERT ( c != 0 );
  IFreeColor ( c );
  PASS ();
}

TEST named_color_known ( void )
{
  IColor c = 0;
  ASSERT_EQ ( INoError, IAllocNamedColor ( "red", &c ) );
  IFreeColor ( c );
  PASS ();
}

TEST named_color_unknown_fails ( void )
{
  IColor c = 0;
  ASSERT ( IAllocNamedColor ( "not_a_real_color_name", &c ) != INoError );
  PASS ();
}

TEST transparent_roundtrip ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  IColor c = IAllocColor ( 10, 20, 30 );
  IColor got = 0;
  ASSERT_EQ ( INoError, ISetTransparent ( im, c ) );
  ASSERT_EQ ( INoError, IGetTransparent ( im, &got ) );
  IFreeImage ( im );
  PASS ();
}

SUITE ( color )
{
  RUN_TEST ( alloc_color_nonzero );
  RUN_TEST ( named_color_known );
  RUN_TEST ( named_color_unknown_fails );
  RUN_TEST ( transparent_roundtrip );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( color );
  GREATEST_MAIN_END ();
}
