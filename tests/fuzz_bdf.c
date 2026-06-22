/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Fuzz harness for the X11 BDF font parser: feed arbitrary bytes to
 * ILoadFontFromFile() (the parser reads a path, so the input is written to a
 * temp file) and ensure no crash / overflow / UB on malformed fonts.
 *
 * Built two ways, like fuzz_decode.c:
 *   - libFuzzer (CI):     clang -fsanitize=fuzzer,address,undefined
 *   - standalone (local): cc -DILIB_FUZZ_STANDALONE -fsanitize=address,undefined
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Ilib.h>

int LLVMFuzzerTestOneInput ( const uint8_t *data, size_t size )
{
  char path[] = "/tmp/ilib_fuzz_bdf_XXXXXX";
  int fd = mkstemp ( path );
  FILE *fp;
  IFont font = NULL;

  if ( fd < 0 )
    return ( 0 );
  fp = fdopen ( fd, "wb" );
  if ( !fp ) {
    close ( fd );
    unlink ( path );
    return ( 0 );
  }
  if ( size )
    fwrite ( data, 1, size, fp );
  fclose ( fp );

  if ( ILoadFontFromFile ( "fuzz", path, &font ) == INoError && font )
    IFreeFont ( font );

  unlink ( path );
  return ( 0 );
}

#ifdef ILIB_FUZZ_STANDALONE
/* Seed with the line keywords the parser dispatches on, so random bytes
   occasionally reach the glyph/bitmap paths. */
static const char *SEED[] = {
  "STARTFONT 2.1\n", "STARTCHAR A\n", "ENCODING 65\n", "BBX ", "BITMAP\n",
  "ENDCHAR\n", "PIXEL_SIZE 200\n", "FONT_ASCENT 8\n", "FACE_NAME ", "FF\n" };

int main ( int argc, char **argv )
{
  int iters = ( argc > 1 ) ? atoi ( argv[1] ) : 50000;
  unsigned int seed = 0xBDF00D;
  uint8_t buf[2048];
  int i;

  for ( i = 0; i < iters; i++ ) {
    size_t n = 0;
    int lines = rand_r ( &seed ) % 24;
    int k;
    for ( k = 0; k < lines && n < sizeof ( buf ) - 64; k++ ) {
      if ( rand_r ( &seed ) & 1 ) {
        const char *p =
          SEED[rand_r ( &seed ) % ( sizeof ( SEED ) / sizeof ( SEED[0] ) )];
        size_t l = strlen ( p );
        memcpy ( buf + n, p, l );
        n += l;
      }
      {
        size_t r = rand_r ( &seed ) % 16, j;
        for ( j = 0; j < r && n < sizeof ( buf ) - 1; j++ )
          buf[n++] = (uint8_t) ( rand_r ( &seed ) & 0xff );
      }
      buf[n++] = '\n';
    }
    LLVMFuzzerTestOneInput ( buf, n );
  }

  printf ( "standalone bdf fuzz: %d iterations, no crash\n", iters );
  return ( 0 );
}
#endif
