/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ianim.c
 *
 * Ilib animation tool (installed as ilib-anim).
 *
 * Description:
 *	Assemble still images into an animated GIF, split an animated GIF back
 *	into per-frame images, or print a summary of an animation.
 *
 *	Usage:
 *	  ilib-anim assemble [--delay MS] [--loop N] -o OUT FRAME...
 *	  ilib-anim split   [--prefix PFX] [--format FMT] IN
 *	  ilib-anim info    IN
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ilib.h>

/* Read a still image, inferring the format from its extension. */
static IImage read_image ( const char *path )
{
  IFileFormat fmt;
  FILE *fp;
  IImage img = NULL;
  IError e;

  if ( IFileType ( (char *) path, &fmt ) != INoError ) {
    fprintf ( stderr, "ilib-anim: cannot determine format of %s\n", path );
    return ( NULL );
  }
  fp = fopen ( path, "rb" );
  if ( !fp ) {
    perror ( path );
    return ( NULL );
  }
  e = IReadImageFile ( fp, fmt, IOPTION_NONE, &img );
  fclose ( fp );
  if ( e != INoError ) {
    fprintf ( stderr, "ilib-anim: reading %s: %s\n", path, IErrorString ( e ) );
    return ( NULL );
  }
  return ( img );
}

/* Map a format name ("png", "ppm", ...) to an IFileFormat via IFileType. */
static IError format_from_name ( const char *name, IFileFormat *out )
{
  char tmp[64];
  snprintf ( tmp, sizeof ( tmp ), "x.%s", name );
  return ( IFileType ( tmp, out ) );
}

static int cmd_assemble ( int argc, char *argv[] )
{
  int delay = 100, loop = 0, i, nframes = 0, rc = 1;
  const char *out = NULL;
  const char **frames;
  IAnimation anim;
  FILE *fp;
  IError e;

  frames = (const char **) malloc ( (size_t) argc * sizeof ( char * ) );
  if ( !frames )
    return ( 1 );

  for ( i = 0; i < argc; i++ ) {
    if ( strcmp ( argv[i], "--delay" ) == 0 && i + 1 < argc )
      delay = atoi ( argv[++i] );
    else if ( strcmp ( argv[i], "--loop" ) == 0 && i + 1 < argc )
      loop = atoi ( argv[++i] );
    else if ( ( strcmp ( argv[i], "-o" ) == 0 ||
                strcmp ( argv[i], "--output" ) == 0 ) &&
              i + 1 < argc )
      out = argv[++i];
    else if ( argv[i][0] == '-' ) {
      fprintf ( stderr, "ilib-anim: unknown option %s\n", argv[i] );
      free ( frames );
      return ( 1 );
    }
    else
      frames[nframes++] = argv[i];
  }

  if ( !out || nframes == 0 ) {
    fprintf ( stderr,
      "Usage: ilib-anim assemble [--delay MS] [--loop N] -o OUT FRAME...\n" );
    free ( frames );
    return ( 1 );
  }

  anim = ICreateAnimation ();
  if ( !anim ) {
    free ( frames );
    return ( 1 );
  }
  for ( i = 0; i < nframes; i++ ) {
    IImage img = read_image ( frames[i] );
    if ( !img )
      goto done;
    e = IAddAnimationFrame ( anim, img, delay );
    IFreeImage ( img );
    if ( e != INoError ) {
      fprintf ( stderr, "ilib-anim: adding frame: %s\n", IErrorString ( e ) );
      goto done;
    }
  }
  ISetAnimationLoopCount ( anim, loop );

  fp = fopen ( out, "wb" );
  if ( !fp ) {
    perror ( out );
    goto done;
  }
  e = IWriteAnimationFile ( fp, anim, IFORMAT_GIF, IOPTION_NONE );
  fclose ( fp );
  if ( e != INoError ) {
    fprintf ( stderr, "ilib-anim: writing %s: %s\n", out, IErrorString ( e ) );
    goto done;
  }
  printf ( "Wrote %s (%d frames).\n", out, nframes );
  rc = 0;

done:
  IFreeAnimation ( anim );
  free ( frames );
  return ( rc );
}

static int cmd_split ( int argc, char *argv[] )
{
  const char *prefix = "frame", *fmtname = "png", *in = NULL;
  int i, n, rc = 1;
  IFileFormat outfmt;
  IAnimation anim;
  FILE *fp;

  for ( i = 0; i < argc; i++ ) {
    if ( strcmp ( argv[i], "--prefix" ) == 0 && i + 1 < argc )
      prefix = argv[++i];
    else if ( strcmp ( argv[i], "--format" ) == 0 && i + 1 < argc )
      fmtname = argv[++i];
    else if ( argv[i][0] == '-' ) {
      fprintf ( stderr, "ilib-anim: unknown option %s\n", argv[i] );
      return ( 1 );
    }
    else
      in = argv[i];
  }
  if ( !in ) {
    fprintf ( stderr,
      "Usage: ilib-anim split [--prefix PFX] [--format FMT] IN\n" );
    return ( 1 );
  }
  if ( format_from_name ( fmtname, &outfmt ) != INoError ) {
    fprintf ( stderr, "ilib-anim: unknown output format %s\n", fmtname );
    return ( 1 );
  }

  fp = fopen ( in, "rb" );
  if ( !fp ) {
    perror ( in );
    return ( 1 );
  }
  anim = IReadAnimationFile ( fp, IFORMAT_GIF );
  fclose ( fp );
  if ( !anim ) {
    fprintf ( stderr, "ilib-anim: could not read animation from %s "
                      "(animated GIF, with giflib?)\n",
      in );
    return ( 1 );
  }

  n = IAnimationFrameCount ( anim );
  for ( i = 0; i < n; i++ ) {
    char name[1024];
    FILE *out;
    IError e;
    snprintf ( name, sizeof ( name ), "%s-%04d.%s", prefix, i + 1, fmtname );
    out = fopen ( name, "wb" );
    if ( !out ) {
      perror ( name );
      goto done;
    }
    e = IWriteImageFile ( out, IAnimationFrame ( anim, i ), outfmt,
      IOPTION_NONE );
    fclose ( out );
    if ( e != INoError ) {
      fprintf ( stderr, "ilib-anim: writing %s: %s\n", name,
        IErrorString ( e ) );
      goto done;
    }
    printf ( "%s\n", name );
  }
  rc = 0;

done:
  IFreeAnimation ( anim );
  return ( rc );
}

static int cmd_info ( int argc, char *argv[] )
{
  const char *in = NULL;
  int i, n;
  IAnimation anim;
  IImage f0;
  FILE *fp;

  for ( i = 0; i < argc; i++ ) {
    if ( argv[i][0] == '-' ) {
      fprintf ( stderr, "ilib-anim: unknown option %s\n", argv[i] );
      return ( 1 );
    }
    in = argv[i];
  }
  if ( !in ) {
    fprintf ( stderr, "Usage: ilib-anim info IN\n" );
    return ( 1 );
  }

  fp = fopen ( in, "rb" );
  if ( !fp ) {
    perror ( in );
    return ( 1 );
  }
  anim = IReadAnimationFile ( fp, IFORMAT_GIF );
  fclose ( fp );
  if ( !anim ) {
    fprintf ( stderr, "ilib-anim: could not read animation from %s\n", in );
    return ( 1 );
  }

  n = IAnimationFrameCount ( anim );
  f0 = IAnimationFrame ( anim, 0 );
  printf ( "file:   %s\n", in );
  printf ( "frames: %d\n", n );
  if ( f0 )
    printf ( "size:   %ux%u\n", IImageWidth ( f0 ), IImageHeight ( f0 ) );
  printf ( "loop:   %d%s\n", IAnimationLoopCount ( anim ),
    IAnimationLoopCount ( anim ) == 0 ? " (forever)" : "" );
  for ( i = 0; i < n; i++ )
    printf ( "  frame %d: %d ms\n", i + 1, IAnimationFrameDelay ( anim, i ) );

  IFreeAnimation ( anim );
  return ( 0 );
}

static void usage ( void )
{
  fprintf ( stderr,
    "Usage: ilib-anim <command> [options]\n"
    "\n"
    "Commands:\n"
    "  assemble [--delay MS] [--loop N] -o OUT FRAME...\n"
    "        Combine still images into an animated GIF (delay default 100ms,\n"
    "        loop default 0 = forever).\n"
    "  split [--prefix PFX] [--format FMT] IN\n"
    "        Write each frame of IN to PFX-NNNN.FMT (defaults frame / png).\n"
    "  info IN\n"
    "        Print the frame count, size, loop count and per-frame delays.\n" );
}

int main ( int argc, char *argv[] )
{
  if ( argc < 2 ) {
    usage ();
    return ( 1 );
  }
  if ( strcmp ( argv[1], "assemble" ) == 0 )
    return ( cmd_assemble ( argc - 2, argv + 2 ) );
  if ( strcmp ( argv[1], "split" ) == 0 )
    return ( cmd_split ( argc - 2, argv + 2 ) );
  if ( strcmp ( argv[1], "info" ) == 0 )
    return ( cmd_info ( argc - 2, argv + 2 ) );
  if ( strcmp ( argv[1], "-h" ) == 0 || strcmp ( argv[1], "--help" ) == 0 ) {
    usage ();
    return ( 0 );
  }
  fprintf ( stderr, "ilib-anim: unknown command %s\n", argv[1] );
  usage ();
  return ( 1 );
}
