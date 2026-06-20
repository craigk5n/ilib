/* SPDX-License-Identifier: GPL-2.0-only */
/* Image format I/O: round-trips for the always-available raw formats,
   graceful handling of malformed input, and clean behaviour for the optional
   codecs whether or not they were compiled in. */

#include <stdio.h>
#include <Ilib.h>
#include "pixutil.h"
#include "greatest.h"

/* Write image to a temp file in `format`, read it back, return the new image
   (or NULL). Sets *write_ret/*read_ret to the IError of each step. */
static IImage roundtrip ( IImage im, IFileFormat format,
  IError *write_ret, IError *read_ret )
{
  IImage back = NULL;
  FILE *fp = tmpfile ();
  if ( !fp ) {
    *write_ret = *read_ret = IErrorWriting;
    return NULL;
  }
  *write_ret = IWriteImageFile ( fp, im, format, IOPTION_NONE );
  rewind ( fp );
  *read_ret = IReadImageFile ( fp, format, IOPTION_NONE, &back );
  fclose ( fp );
  return back;
}

TEST ppm_roundtrip ( void )
{
  IImage im = ICreateImage ( 6, 4, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor red = IAllocColor ( 255, 0, 0 );
  IError wr, rd;
  IImage back;

  ISetForeground ( gc, red );
  IDrawPoint ( im, gc, 2, 1 );

  back = roundtrip ( im, IFORMAT_PPM, &wr, &rd );
  ASSERT_EQ ( INoError, wr );
  ASSERT_EQ ( INoError, rd );
  ASSERT ( back != NULL );
  ASSERT_EQ ( 6, px_width ( back ) );
  ASSERT_EQ ( 4, px_height ( back ) );
  ASSERT_EQ ( 255, px_r ( back, 2, 1 ) );
  ASSERT_EQ ( 0, px_g ( back, 2, 1 ) );
  ASSERT_EQ ( 0, px_b ( back, 2, 1 ) );

  IFreeImage ( back );
  IFreeColor ( red );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

TEST pgm_roundtrip_greyscale ( void )
{
  IImage im = ICreateImage ( 5, 5, IOPTION_GREYSCALE );
  IGC gc = ICreateGC ();
  IColor grey = IAllocColor ( 128, 128, 128 );
  IError wr, rd;
  IImage back;

  ISetForeground ( gc, grey );
  IDrawPoint ( im, gc, 3, 3 );

  back = roundtrip ( im, IFORMAT_PGM, &wr, &rd );
  ASSERT_EQ ( INoError, wr );
  ASSERT_EQ ( INoError, rd );
  ASSERT ( back != NULL );
  ASSERT_EQ ( 5, px_width ( back ) );
  ASSERT_EQ ( 128, px_r ( back, 3, 3 ) );

  IFreeImage ( back );
  IFreeColor ( grey );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();
}

/* Feeding garbage to a decoder must return an error, not crash. */
TEST garbage_ppm_rejected ( void )
{
  FILE *fp = tmpfile ();
  IImage back = NULL;
  IError rd;
  fputs ( "this is definitely not a PPM image file\n", fp );
  rewind ( fp );
  rd = IReadImageFile ( fp, IFORMAT_PPM, IOPTION_NONE, &back );
  fclose ( fp );
  ASSERT ( rd != INoError );
  if ( back )
    IFreeImage ( back );
  PASS ();
}

/* A truncated header must also be rejected cleanly. */
TEST truncated_ppm_rejected ( void )
{
  FILE *fp = tmpfile ();
  IImage back = NULL;
  IError rd;
  fputs ( "P6\n", fp );
  rewind ( fp );
  rd = IReadImageFile ( fp, IFORMAT_PPM, IOPTION_NONE, &back );
  fclose ( fp );
  ASSERT ( rd != INoError );
  if ( back )
    IFreeImage ( back );
  PASS ();
}

/* Writing an optional codec must either succeed (when compiled in) or fail
   cleanly (when not) -- never crash. When it round-trips, the image must come
   back intact. */
TEST optional_codec_write_is_clean ( void )
{
  IImage im = ICreateImage ( 4, 4, IOPTION_NONE );
  IError wr, rd;
  IImage back = roundtrip ( im, IFORMAT_PNG, &wr, &rd );
  if ( wr == INoError ) {
    ASSERT_EQ ( INoError, rd );
    ASSERT ( back != NULL );
    ASSERT_EQ ( 4, px_width ( back ) );
    IFreeImage ( back );
  }
  else {
    /* not compiled in: a clean error, and no image produced */
    ASSERT ( back == NULL );
  }
  IFreeImage ( im );
  PASS ();
}

SUITE ( formats )
{
  RUN_TEST ( ppm_roundtrip );
  RUN_TEST ( pgm_roundtrip_greyscale );
  RUN_TEST ( garbage_ppm_rejected );
  RUN_TEST ( truncated_ppm_rejected );
  RUN_TEST ( optional_codec_write_is_clean );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( formats );
  GREATEST_MAIN_END ();
}
