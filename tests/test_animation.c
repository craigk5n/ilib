/* SPDX-License-Identifier: GPL-2.0-only */
/* Animation (multi-frame / animated GIF) tests. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Ilib.h>
#include "greatest.h"

static IImage solid ( int w, int h, unsigned int r, unsigned int g,
  unsigned int b )
{
  IImage im = ICreateImage ( w, h, IOPTION_NONE );
  int x, y;
  for ( y = 0; y < h; y++ )
    for ( x = 0; x < w; x++ )
      ISetPixel ( im, x, y, r, g, b );
  return ( im );
}

TEST animation_build_and_query ( void )
{
  IAnimation a = ICreateAnimation ();
  IImage f0 = solid ( 8, 6, 255, 0, 0 );
  IImage f1 = solid ( 8, 6, 0, 0, 255 );

  ASSERT ( a != NULL );
  ASSERT_EQ ( 0, IAnimationFrameCount ( a ) );
  ASSERT_EQ ( INoError, IAddAnimationFrame ( a, f0, 100 ) );
  ASSERT_EQ ( INoError, IAddAnimationFrame ( a, f1, 200 ) );
  ASSERT_EQ ( 2, IAnimationFrameCount ( a ) );
  ASSERT_EQ ( 100, IAnimationFrameDelay ( a, 0 ) );
  ASSERT_EQ ( 200, IAnimationFrameDelay ( a, 1 ) );
  ASSERT ( IAnimationFrame ( a, 0 ) != NULL );
  ASSERT ( IAnimationFrame ( a, 5 ) == NULL ); /* out of range */
  ASSERT_EQ ( INoError, ISetAnimationLoopCount ( a, 3 ) );
  ASSERT_EQ ( 3, IAnimationLoopCount ( a ) );

  /* The animation owns a copy; freeing our originals is independent. */
  IFreeImage ( f0 );
  IFreeImage ( f1 );
  IFreeAnimation ( a );
  ASSERT ( a == NULL ); /* the free macro nulls the handle */
  PASS ();
}

TEST animation_rejects_bad_handle ( void )
{
  IImage f = solid ( 4, 4, 1, 2, 3 );
  ASSERT_EQ ( IInvalidAnimation, IAddAnimationFrame ( NULL, f, 10 ) );
  ASSERT_EQ ( IInvalidAnimation, ISetAnimationLoopCount ( NULL, 1 ) );
  ASSERT_EQ ( 0, IAnimationFrameCount ( NULL ) );
  ASSERT_EQ ( 0, IAnimationFrameDelay ( NULL, 0 ) );
  ASSERT ( IAnimationFrame ( NULL, 0 ) == NULL );
  IFreeImage ( f );
  PASS ();
}

TEST animation_gif_round_trip ( void )
{
  IAnimation a = ICreateAnimation ();
  IImage f0 = solid ( 12, 10, 255, 0, 0 );
  IImage f1 = solid ( 12, 10, 0, 0, 255 );
  char path[] = "/tmp/ilib_anim_XXXXXX";
  int fd;
  FILE *fp;
  IError w;
  IAnimation b;
  unsigned int r, g, bl;

  IAddAnimationFrame ( a, f0, 100 );
  IAddAnimationFrame ( a, f1, 200 );
  ISetAnimationLoopCount ( a, 5 );
  IFreeImage ( f0 );
  IFreeImage ( f1 );

  /* Use a real path: giflib closes the fd on write, so we reopen to read. */
  fd = mkstemp ( path );
  ASSERT ( fd >= 0 );
  fp = fdopen ( fd, "wb" );
  ASSERT ( fp != NULL );
  w = IWriteAnimationFile ( fp, a, IFORMAT_GIF, IOPTION_NONE );
  fclose ( fp ); /* fd already closed by giflib; frees the FILE struct */
  IFreeAnimation ( a );
  if ( w == INoGIFSupport ) {
    unlink ( path );
    SKIP (); /* library built without giflib */
  }
  ASSERT_EQ ( INoError, w );

  fp = fopen ( path, "rb" );
  ASSERT ( fp != NULL );
  b = IReadAnimationFile ( fp, IFORMAT_GIF );
  fclose ( fp );
  unlink ( path );
  ASSERT ( b != NULL );
  ASSERT_EQ ( 2, IAnimationFrameCount ( b ) );
  ASSERT_EQ ( 100, IAnimationFrameDelay ( b, 0 ) );
  ASSERT_EQ ( 200, IAnimationFrameDelay ( b, 1 ) );
  ASSERT_EQ ( 5, IAnimationLoopCount ( b ) );
  ASSERT_EQ ( 12, (int) IImageWidth ( IAnimationFrame ( b, 0 ) ) );

  /* Solid colors survive the palette round-trip exactly. */
  IGetPixel ( IAnimationFrame ( b, 0 ), 6, 5, &r, &g, &bl );
  ASSERT_EQ ( 255, r );
  ASSERT_EQ ( 0, g );
  ASSERT_EQ ( 0, bl );
  IGetPixel ( IAnimationFrame ( b, 1 ), 6, 5, &r, &g, &bl );
  ASSERT_EQ ( 0, r );
  ASSERT_EQ ( 255, bl );

  IFreeAnimation ( b );
  PASS ();
}

SUITE ( animation_suite )
{
  RUN_TEST ( animation_build_and_query );
  RUN_TEST ( animation_rejects_bad_handle );
  RUN_TEST ( animation_gif_round_trip );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( animation_suite );
  GREATEST_MAIN_END ();
}
