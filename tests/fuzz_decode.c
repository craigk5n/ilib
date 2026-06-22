/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Fuzz harness for the image decoders: feed arbitrary bytes to
 * IReadImageFile() for every format and ensure no crash / overflow / UB.
 *
 * Built two ways:
 *   - libFuzzer (CI):    clang -fsanitize=fuzzer,address,undefined
 *                        (defines the entry point LLVMFuzzerTestOneInput)
 *   - standalone (local): cc -DILIB_FUZZ_STANDALONE -fsanitize=address,undefined
 *                        runs deterministic pseudo-random + seeded inputs so
 *                        crashes can be shaken out / reproduced without clang.
 *
 * Decoder memory leaks on malformed input are a known, separate issue, so the
 * fuzz job runs with leak detection disabled; this harness targets crashes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <Ilib.h>

static const IFileFormat ALL_FORMATS[] = {
  IFORMAT_PPM, IFORMAT_PGM, IFORMAT_PBM, IFORMAT_XPM, IFORMAT_XBM,
  IFORMAT_PNG, IFORMAT_JPEG, IFORMAT_GIF, IFORMAT_BMP, IFORMAT_WEBP,
  IFORMAT_AVIF, IFORMAT_TIFF };

/* These decoders use fileno()/dup() or otherwise need a real backing fd rather
   than an fmemopen() buffer. */
static int needs_real_fd ( IFileFormat fmt )
{
  return ( fmt == IFORMAT_GIF || fmt == IFORMAT_WEBP || fmt == IFORMAT_AVIF ||
    fmt == IFORMAT_TIFF );
}

static void try_one ( const uint8_t *data, size_t size, IFileFormat fmt )
{
  IImage img = NULL;
  FILE *fp;

  if ( needs_real_fd ( fmt ) ) {
    fp = tmpfile ();
    if ( !fp )
      return;
    if ( size )
      fwrite ( data, 1, size, fp );
    rewind ( fp );
  }
  else {
    /* everyone else reads via stdio: an in-memory stream is much faster */
    fp = fmemopen ( (void *) data, size, "rb" );
    if ( !fp )
      return; /* e.g. size == 0 */
  }

  if ( IReadImageFile ( fp, fmt, IOPTION_NONE, &img ) == INoError && img )
    IFreeImage ( img );
  fclose ( fp );
}

int LLVMFuzzerTestOneInput ( const uint8_t *data, size_t size )
{
  size_t i;
  for ( i = 0; i < sizeof ( ALL_FORMATS ) / sizeof ( ALL_FORMATS[0] ); i++ )
    try_one ( data, size, ALL_FORMATS[i] );
  return 0;
}

#ifdef ILIB_FUZZ_STANDALONE
/* A few magic prefixes so random bytes occasionally get past the format
   sniffing and exercise real decode paths. */
static const char *SEED_PREFIXES[] = { "P6\n", "P5\n", "P4\n", "/* XPM */\n",
  "BM", "\x89PNG\r\n\x1a\n", "GIF89a", "\xff\xd8\xff" };

int main ( int argc, char **argv )
{
  int iters = ( argc > 1 ) ? atoi ( argv[1] ) : 100000;
  unsigned int seed = 0xC0FFEEu;
  uint8_t buf[1024];
  int i;

  for ( i = 0; i < iters; i++ ) {
    size_t n, j, off = 0;
    /* half the time, start with a real format prefix */
    if ( rand_r ( &seed ) & 1 ) {
      const char *p = SEED_PREFIXES[rand_r ( &seed ) %
                                    ( sizeof ( SEED_PREFIXES ) / sizeof ( SEED_PREFIXES[0] ) )];
      off = strlen ( p );
      memcpy ( buf, p, off );
    }
    n = off + ( rand_r ( &seed ) % ( sizeof ( buf ) - off ) );
    for ( j = off; j < n; j++ )
      buf[j] = (uint8_t) ( rand_r ( &seed ) & 0xff );
    LLVMFuzzerTestOneInput ( buf, n );
  }

  printf ( "standalone fuzz: %d iterations, no crash\n", iters );
  return 0;
}
#endif
