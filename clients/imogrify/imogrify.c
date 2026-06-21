/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * imogrify.c
 *
 * Image library -- client tool (installed as ilib-mogrify)
 *
 * Description:
 *	Batch image editor in the spirit of ImageMagick's `mogrify`. Applies a
 *	pipeline of operations (in command-line order) to each named file and
 *	writes the result back to the SAME file (overwriting it in place),
 *	keeping each file's own format. With --format EXT, results are written
 *	to new files with that extension instead, leaving the originals intact.
 *
 *	Usage:  ilib-mogrify [options] file1 [file2 ...]
 *
 ****************************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ilib.h>

typedef enum {
  OP_GREYSCALE,
  OP_NEGATE,
  OP_BRIGHTNESS,
  OP_CONTRAST,
  OP_GAMMA,
  OP_THRESHOLD,
  OP_FLIP,
  OP_FLOP,
  OP_ROTATE,
  OP_BLUR,
  OP_GAUSSIAN_BLUR,
  OP_SHARPEN,
  OP_EDGE,
  OP_EMBOSS,
  OP_RESIZE,
  OP_REDUCE_COLORS,
  OP_NORMALIZE,
  OP_SEPIA,
  OP_OPACITY,
  OP_TRIM,
  OP_BORDER
} OpType;

typedef struct {
  OpType type;
  int i1, i2;
  double d1;
} Op;

#define MAX_OPS 128

static void usage ( const char *prog )
{
  fprintf ( stderr,
    "Usage: %s [options] file1 [file2 ...]\n"
    "\n"
    "Applies the given operations (in order) to each file and writes the\n"
    "result back to the same file in place. With --format EXT, writes new\n"
    "files with that extension instead, leaving the originals intact.\n"
    "\n"
    "Options:\n"
    "  --format EXT               write new <name>.EXT files instead of\n"
    "                             overwriting (e.g. --format png)\n"
    "  --greyscale, --grayscale   desaturate to grey (Rec.601 luma)\n"
    "  --negate                   invert colours\n"
    "  --brightness N             adjust brightness (-100..100)\n"
    "  --contrast N               adjust contrast (-100..100)\n"
    "  --gamma F                  gamma correction (F > 0)\n"
    "  --threshold N              threshold to black/white at N (0..255)\n"
    "  --flip                     mirror vertically\n"
    "  --flop                     mirror horizontally\n"
    "  --rotate DEG               rotate clockwise DEG degrees\n"
    "  --background NAME          fill colour for non-90 rotations "
    "(default white)\n"
    "  --blur R                   box blur, radius R\n"
    "  --gaussian-blur SIGMA      Gaussian blur, standard deviation SIGMA\n"
    "  --sharpen                  sharpen\n"
    "  --edge                     edge detect\n"
    "  --emboss                   emboss\n"
    "  --resize WxH               resize to WxH (bilinear)\n"
    "  --reduce-colors N          reduce to at most N colours\n"
    "  --dither                   Floyd-Steinberg dither on GIF color reduction\n"
    "  --normalize                auto-stretch contrast\n"
    "  --sepia                    apply a sepia tone\n"
    "  --opacity F                scale RGBA alpha by F (e.g. 0.5)\n"
    "  --trim                     autocrop a uniform border\n"
    "  --border N                 add an N-pixel border (uses --background)\n"
    "  -h, --help                 show this help\n",
    prog );
}

static int is_flag ( const char *tok, const char *name )
{
  const char *t;
  if ( tok[0] != '-' )
    return ( 0 );
  t = tok + 1;
  if ( *t == '-' )
    t++;
  return ( strcmp ( t, name ) == 0 );
}

static const char *next_arg ( int *loop, int argc, char *argv[],
  const char *flag )
{
  if ( *loop + 1 >= argc ) {
    fprintf ( stderr, "Option %s requires an argument.\n", flag );
    exit ( 1 );
  }
  return ( argv[++( *loop )] );
}

/* Return a new string with path's extension replaced by ext (caller frees). */
static char *replace_ext ( const char *path, const char *ext )
{
  const char *dot = strrchr ( path, '.' );
  const char *slash = strrchr ( path, '/' );
  size_t base_len;
  char *out;

  if ( dot && ( !slash || dot > slash ) )
    base_len = (size_t) ( dot - path );
  else
    base_len = strlen ( path );

  out = (char *) malloc ( base_len + strlen ( ext ) + 2 );
  if ( !out )
    return ( NULL );
  memcpy ( out, path, base_len );
  out[base_len] = '.';
  strcpy ( out + base_len + 1, ext );
  return ( out );
}

/* Apply one operation; returns the IError. */
static IError apply_op ( IImage image, const Op *op, const char *background )
{
  switch ( op->type ) {
  case OP_GREYSCALE:
    return ( IGreyscale ( image ) );
  case OP_NEGATE:
    return ( INegate ( image ) );
  case OP_BRIGHTNESS:
    return ( IBrightnessContrast ( image, op->i1, 0 ) );
  case OP_CONTRAST:
    return ( IBrightnessContrast ( image, 0, op->i1 ) );
  case OP_GAMMA:
    return ( IGamma ( image, op->d1 ) );
  case OP_THRESHOLD:
    return ( IThreshold ( image, (unsigned int) op->i1 ) );
  case OP_FLIP:
    return ( IFlip ( image ) );
  case OP_FLOP:
    return ( IFlop ( image ) );
  case OP_ROTATE:
    if ( fmod ( op->d1, 90.0 ) == 0.0 )
      return ( IRotate ( image, (int) op->d1 ) );
    else {
      IColor bg;
      /* IAllocNamedColor only reads the name; cast away const safely. */
      if ( IAllocNamedColor ( (char *) background, &bg ) != INoError )
        bg = IAllocColor ( 255, 255, 255 );
      return ( IRotateAngle ( image, op->d1, bg ) );
    }
  case OP_BLUR:
    return ( IBlur ( image, (unsigned int) op->i1 ) );
  case OP_GAUSSIAN_BLUR:
    return ( IGaussianBlur ( image, op->d1 ) );
  case OP_SHARPEN:
    return ( ISharpen ( image ) );
  case OP_EDGE:
    return ( IEdgeDetect ( image ) );
  case OP_EMBOSS:
    return ( IEmboss ( image ) );
  case OP_RESIZE:
    return ( IResizeFiltered ( image, (unsigned int) op->i1,
      (unsigned int) op->i2, IRESIZE_AUTO ) );
  case OP_REDUCE_COLORS:
    return ( IReduceColors ( image, (unsigned int) op->i1 ) );
  case OP_NORMALIZE:
    return ( INormalize ( image ) );
  case OP_SEPIA:
    return ( ISepia ( image ) );
  case OP_OPACITY:
    return ( IOpacity ( image, op->d1 ) );
  case OP_TRIM:
    return ( ITrim ( image, (unsigned int) op->i1 ) );
  case OP_BORDER: {
    IColor bg;
    if ( IAllocNamedColor ( (char *) background, &bg ) != INoError )
      bg = IAllocColor ( 255, 255, 255 );
    return ( IBorder ( image, (unsigned int) op->i1, bg ) );
  }
  }
  return ( INoError );
}

/* Process a single file. Returns 0 on success, non-zero on error. */
static int mogrify_file ( const char *path, const Op *ops, int nops,
  const char *format_ext, const char *background, int dither )
{
  IImage image;
  IFileFormat input_format, output_format;
  char *outpath;
  FILE *fp;
  IError ret;
  int i, rc = 0;

  ret = IFileType ( (char *) path, &input_format );
  if ( ret != INoError ) {
    /* Unknown extension, or a recognized format this build can't read. */
    fprintf ( stderr, "%s: %s\n", path, IErrorString ( ret ) );
    return ( 1 );
  }

  fp = fopen ( path, "rb" );
  if ( !fp ) {
    fprintf ( stderr, "%s: cannot open for reading.\n", path );
    return ( 1 );
  }
  ret = IReadImageFile ( fp, input_format, IOPTION_NONE, &image );
  fclose ( fp );
  if ( ret != INoError ) {
    fprintf ( stderr, "%s: read error: %s\n", path, IErrorString ( ret ) );
    return ( 1 );
  }

  for ( i = 0; i < nops; i++ ) {
    ret = apply_op ( image, &ops[i], background );
    if ( ret != INoError ) {
      fprintf ( stderr, "%s: operation failed: %s\n", path,
        IErrorString ( ret ) );
      IFreeImage ( image );
      return ( 1 );
    }
  }

  /* Choose output path/format: in place, or a new file with --format. */
  if ( format_ext ) {
    char tmp[64];
    snprintf ( tmp, sizeof ( tmp ), "x.%s", format_ext );
    ret = IFileType ( tmp, &output_format );
    if ( ret == IInvalidFormat ) {
      fprintf ( stderr, "Unknown output format: %s\n", format_ext );
      IFreeImage ( image );
      return ( 1 );
    }
    if ( ret != INoError ) {
      /* Recognized format, but unsupported in this build. */
      fprintf ( stderr, "%s format: %s\n", format_ext, IErrorString ( ret ) );
      IFreeImage ( image );
      return ( 1 );
    }
    outpath = replace_ext ( path, format_ext );
    if ( !outpath ) {
      IFreeImage ( image );
      return ( 1 );
    }
  }
  else {
    output_format = input_format;
    outpath = (char *) path; /* overwrite in place; not freed */
  }

  fp = fopen ( outpath, "wb" );
  if ( !fp ) {
    fprintf ( stderr, "%s: cannot open for writing.\n", outpath );
    rc = 1;
  }
  else {
    ret = IWriteImageFile ( fp, image, output_format,
      IOPTION_INTERLACED | ( dither ? IOPTION_DITHER : 0 ) );
    fclose ( fp );
    if ( ret != INoError ) {
      fprintf ( stderr, "%s: write error: %s\n", outpath,
        IErrorString ( ret ) );
      rc = 1;
    }
  }

  if ( format_ext )
    free ( outpath );
  IFreeImage ( image );
  return ( rc );
}

int main ( int argc, char *argv[] )
{
  const char *background = "white";
  const char *format_ext = NULL;
  Op ops[MAX_OPS];
  int nops = 0;
  int nfiles = 0;
  int loop;
  int exit_code = 0;
  int dither = 0;

  /* First pass: collect options and count files. Files are processed in a
     second pass so that all the (order-sensitive) operations are known. */
  for ( loop = 1; loop < argc; loop++ ) {
    char *tok = argv[loop];
    Op op;
    memset ( &op, 0, sizeof ( op ) );

    if ( is_flag ( tok, "help" ) || is_flag ( tok, "h" ) ) {
      usage ( argv[0] );
      return ( 0 );
    }
    else if ( is_flag ( tok, "format" ) ) {
      format_ext = next_arg ( &loop, argc, argv, "--format" );
      continue;
    }
    else if ( is_flag ( tok, "background" ) ) {
      background = next_arg ( &loop, argc, argv, "--background" );
      continue;
    }
    else if ( is_flag ( tok, "greyscale" ) || is_flag ( tok, "grayscale" ) ) {
      op.type = OP_GREYSCALE;
    }
    else if ( is_flag ( tok, "negate" ) ) {
      op.type = OP_NEGATE;
    }
    else if ( is_flag ( tok, "brightness" ) ) {
      op.type = OP_BRIGHTNESS;
      op.i1 = atoi ( next_arg ( &loop, argc, argv, "--brightness" ) );
    }
    else if ( is_flag ( tok, "contrast" ) ) {
      op.type = OP_CONTRAST;
      op.i1 = atoi ( next_arg ( &loop, argc, argv, "--contrast" ) );
    }
    else if ( is_flag ( tok, "gamma" ) ) {
      op.type = OP_GAMMA;
      op.d1 = atof ( next_arg ( &loop, argc, argv, "--gamma" ) );
    }
    else if ( is_flag ( tok, "threshold" ) ) {
      op.type = OP_THRESHOLD;
      op.i1 = atoi ( next_arg ( &loop, argc, argv, "--threshold" ) );
    }
    else if ( is_flag ( tok, "flip" ) ) {
      op.type = OP_FLIP;
    }
    else if ( is_flag ( tok, "flop" ) ) {
      op.type = OP_FLOP;
    }
    else if ( is_flag ( tok, "rotate" ) ) {
      op.type = OP_ROTATE;
      op.d1 = atof ( next_arg ( &loop, argc, argv, "--rotate" ) );
    }
    else if ( is_flag ( tok, "blur" ) ) {
      op.type = OP_BLUR;
      op.i1 = atoi ( next_arg ( &loop, argc, argv, "--blur" ) );
    }
    else if ( is_flag ( tok, "gaussian-blur" ) ) {
      op.type = OP_GAUSSIAN_BLUR;
      op.d1 = atof ( next_arg ( &loop, argc, argv, "--gaussian-blur" ) );
    }
    else if ( is_flag ( tok, "sharpen" ) ) {
      op.type = OP_SHARPEN;
    }
    else if ( is_flag ( tok, "edge" ) || is_flag ( tok, "edge-detect" ) ) {
      op.type = OP_EDGE;
    }
    else if ( is_flag ( tok, "emboss" ) ) {
      op.type = OP_EMBOSS;
    }
    else if ( is_flag ( tok, "resize" ) ) {
      const char *arg = next_arg ( &loop, argc, argv, "--resize" );
      unsigned int w = 0, h = 0;
      if ( sscanf ( arg, "%ux%u", &w, &h ) != 2 || w == 0 || h == 0 ) {
        fprintf ( stderr, "--resize expects WxH (e.g. 320x240).\n" );
        return ( 1 );
      }
      op.type = OP_RESIZE;
      op.i1 = (int) w;
      op.i2 = (int) h;
    }
    else if ( is_flag ( tok, "reduce-colors" ) ||
              is_flag ( tok, "reduce-colours" ) ) {
      op.type = OP_REDUCE_COLORS;
      op.i1 = atoi ( next_arg ( &loop, argc, argv, "--reduce-colors" ) );
    }
    else if ( is_flag ( tok, "normalize" ) || is_flag ( tok, "normalise" ) ) {
      op.type = OP_NORMALIZE;
    }
    else if ( is_flag ( tok, "sepia" ) ) {
      op.type = OP_SEPIA;
    }
    else if ( is_flag ( tok, "opacity" ) ) {
      op.type = OP_OPACITY;
      op.d1 = atof ( next_arg ( &loop, argc, argv, "--opacity" ) );
    }
    else if ( is_flag ( tok, "trim" ) ) {
      op.type = OP_TRIM;
    }
    else if ( is_flag ( tok, "border" ) ) {
      op.type = OP_BORDER;
      op.i1 = atoi ( next_arg ( &loop, argc, argv, "--border" ) );
    }
    else if ( is_flag ( tok, "dither" ) ) {
      dither = 1; /* a write option, not a pipeline op */
      continue;
    }
    else if ( tok[0] == '-' && tok[1] != '\0' ) {
      fprintf ( stderr, "Unknown option: %s\n", tok );
      usage ( argv[0] );
      return ( 1 );
    }
    else {
      nfiles++; /* a file; processed in the second pass */
      continue;
    }

    if ( nops >= MAX_OPS ) {
      fprintf ( stderr, "Too many operations (max %d).\n", MAX_OPS );
      return ( 1 );
    }
    ops[nops++] = op;
  }

  if ( nfiles == 0 ) {
    fprintf ( stderr, "No input files specified.\n" );
    usage ( argv[0] );
    return ( 1 );
  }

  /* Second pass: process each file. */
  for ( loop = 1; loop < argc; loop++ ) {
    char *tok = argv[loop];
    /* Skip options and any argument they consumed. */
    if ( tok[0] == '-' && tok[1] != '\0' ) {
      if ( is_flag ( tok, "format" ) || is_flag ( tok, "background" ) ||
           is_flag ( tok, "brightness" ) || is_flag ( tok, "contrast" ) ||
           is_flag ( tok, "gamma" ) || is_flag ( tok, "threshold" ) ||
           is_flag ( tok, "rotate" ) || is_flag ( tok, "blur" ) ||
           is_flag ( tok, "gaussian-blur" ) || is_flag ( tok, "resize" ) ||
           is_flag ( tok, "reduce-colors" ) ||
           is_flag ( tok, "reduce-colours" ) || is_flag ( tok, "opacity" ) ||
           is_flag ( tok, "border" ) )
        loop++; /* this flag took an argument */
      continue;
    }
    if ( mogrify_file ( tok, ops, nops, format_ext, background, dither ) != 0 )
      exit_code = 1;
  }

  return ( exit_code );
}
