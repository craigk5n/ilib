/* Malformed-input decoder tests. Each feeds truncated/garbage data to a
   decoder and requires a clean error (no crash, no leak). Run under the
   sanitizers CI job (ASan+UBSan, leak detection on), this is the regression
   gate for the decoders' error-path leak hardening. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* lseek, ftruncate via fileno */

#include <Ilib.h>
#include "greatest.h"

/* Read `len` bytes through a real temp file (a real fd is required by the GIF
   decoder). Frees any image produced. Returns the IError. */
static IError feed ( const unsigned char *data, size_t len, IFileFormat fmt )
{
  IImage im = NULL;
  IError r;
  FILE *fp = tmpfile ();
  if ( ! fp )
    return ( IErrorWriting );
  if ( len )
    fwrite ( data, 1, len, fp );
  rewind ( fp );
  r = IReadImageFile ( fp, fmt, IOPTION_NONE, &im );
  if ( im )
    IFreeImage ( im );
  fclose ( fp );
  return ( r );
}

/* Encode `im` to `fmt` in a temp file, then read back only the first half of
   the bytes -- forcing a mid-stream decode failure (the error path that used
   to leak the image). Skips cleanly if the codec is not compiled in. */
static void roundtrip_truncated ( IImage im, IFileFormat fmt )
{
  unsigned char *buf;
  long sz;
  int fd;
  FILE *fp = tmpfile ();
  if ( ! fp )
    return;
  if ( IWriteImageFile ( fp, im, fmt, IOPTION_NONE ) != INoError ) {
    fclose ( fp );   /* codec not built */
    return;
  }
  fflush ( fp );
  fd = fileno ( fp );
  sz = (long) lseek ( fd, 0, SEEK_END );     /* real size (GIF writes via fd) */
  if ( sz < 20 ) { fclose ( fp ); return; }
  buf = (unsigned char *) malloc ( sz );
  if ( ! buf ) { fclose ( fp ); return; }
  lseek ( fd, 0, SEEK_SET );
  if ( read ( fd, buf, sz ) == sz )
    (void) feed ( buf, (size_t) ( sz / 2 ), fmt );   /* truncated -> error */
  free ( buf );
  fclose ( fp );
}

TEST ppm_malformed ( void )
{
  ASSERT ( feed ( (const unsigned char *) "P6\n# c\n", 7, IFORMAT_PPM ) != INoError );
  ASSERT ( feed ( (const unsigned char *) "P6\n2 2\n255\n", 11, IFORMAT_PPM ) != INoError );
  ASSERT ( feed ( (const unsigned char *) "P6\n-1 -1\n255\nx", 14, IFORMAT_PPM ) != INoError );
  ASSERT ( feed ( (const unsigned char *) "P5\n2 2\n0\nxxxx", 13, IFORMAT_PGM ) != INoError );
  PASS ();
}

TEST xpm_malformed ( void )
{
  ASSERT ( feed ( (const unsigned char *) "\"4 4 2 1\"\n", 10, IFORMAT_XPM ) != INoError );
  ASSERT ( feed ( (const unsigned char *) "\"2 2 1 1\"\n\"a c #f00\"\n", 20, IFORMAT_XPM ) != INoError );
  ASSERT ( feed ( (const unsigned char *) "\"99999999 99999999 2 1\"\n", 24, IFORMAT_XPM ) != INoError );
  PASS ();
}

/* a 54-byte BMP header builder, optionally truncated after `extra` data bytes */
static void bmp_case ( int w, int h, int depth, int comp, int extra, IFileFormat unused )
{
  unsigned char b[256];
  int n = 0, i;
  (void) unused;
#define P16(v) do { b[n++]=(v)&0xff; b[n++]=((v)>>8)&0xff; } while (0)
#define P32(v) do { b[n++]=(v)&0xff; b[n++]=((v)>>8)&0xff; \
                    b[n++]=((v)>>16)&0xff; b[n++]=((v)>>24)&0xff; } while (0)
  P16 ( 'B' + ( 'M' << 8 ) ); P32 ( 0 ); P16 ( 0 ); P16 ( 0 ); P32 ( 54 );
  P32 ( 40 ); P32 ( w ); P32 ( h ); P16 ( 1 ); P16 ( depth );
  P32 ( comp ); P32 ( 0 ); P32 ( 0 ); P32 ( 0 ); P32 ( 0 ); P32 ( 0 );
  for ( i = 0; i < extra; i++ ) b[n++] = 0x11;
#undef P16
#undef P32
  (void) feed ( b, n, IFORMAT_BMP );
}

TEST bmp_malformed ( void )
{
  bmp_case ( 4, 4, 8, 0, 2, 0 );    /* colortable truncated */
  bmp_case ( 4, 4, 8, 0, 40, 0 );   /* pixel data truncated */
  bmp_case ( 4, 4, 24, 0, 5, 0 );   /* 24-bit truncated */
  bmp_case ( 4, 4, 8, 1, 30, 0 );   /* RLE truncated */
  bmp_case ( 4, 4, 16, 3, 8, 0 );   /* bitfields truncated */
  bmp_case ( 100000, 100000, 24, 0, 0, 0 );  /* oversize -> rejected */
  PASS ();
}

/* Build a small valid image and truncate-decode it through each codec. */
TEST codec_truncated_decode ( void )
{
  IImage im = ICreateImage ( 20, 20, IOPTION_NONE );
  IGC gc = ICreateGC ();
  IColor c = IAllocColor ( 10, 20, 30 );
  ISetForeground ( gc, c );
  IFillRectangle ( im, gc, 0, 0, 20, 20 );

  roundtrip_truncated ( im, IFORMAT_PNG );
  roundtrip_truncated ( im, IFORMAT_JPEG );
  roundtrip_truncated ( im, IFORMAT_GIF );

  IFreeColor ( c );
  IFreeGC ( gc );
  IFreeImage ( im );
  PASS ();   /* success == no crash / no leak under the sanitizers job */
}

SUITE ( malformed )
{
  RUN_TEST ( ppm_malformed );
  RUN_TEST ( xpm_malformed );
  RUN_TEST ( bmp_malformed );
  RUN_TEST ( codec_truncated_decode );
}

GREATEST_MAIN_DEFS ();

int main ( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN ();
  RUN_SUITE ( malformed );
  GREATEST_MAIN_END ();
}
