/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * iconvert.c
 *
 * Image library -- example client
 *
 * Description:
 *	Convert an image between formats (chosen by file extension) and,
 *	optionally, apply a pipeline of image operations in command-line order
 *	(greyscale, negate, brightness/contrast, gamma, threshold, flip, flop,
 *	rotate, blur, sharpen, edge detect, emboss, resize, colour reduction).
 *
 *	Usage:  ilib-convert [options] infile outfile
 *
 ****************************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Ilib.h>

/* The operation pipeline, recorded during argument parsing and applied to the
   image in order after it is read. */
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
    "Usage: %s [options] infile outfile\n"
    "\n"
    "Converts infile to outfile (formats chosen by file extension) and\n"
    "applies any options, in the order given, in between.\n"
    "\n"
    "Options:\n"
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
    "  --auto-orient              apply the JPEG's EXIF orientation on load\n"
    "  --normalize                auto-stretch contrast\n"
    "  --sepia                    apply a sepia tone\n"
    "  --opacity F                scale RGBA alpha by F (e.g. 0.5)\n"
    "  --trim                     autocrop a uniform border\n"
    "  --border N                 add an N-pixel border (uses --background)\n"
    "  -h, --help                 show this help\n",
    prog );
}

/* Match tok against "-name" or "--name". */
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

/* Return the argument following a flag, or exit if it is missing. */
static const char *next_arg ( int *loop, int argc, char *argv[],
  const char *flag )
{
  if ( *loop + 1 >= argc ) {
    fprintf ( stderr, "Option %s requires an argument.\n", flag );
    exit ( 1 );
  }
  return ( argv[++( *loop )] );
}

static void apply_op ( IImage image, const Op *op, const char *background )
{
  IError ret = INoError;

  switch ( op->type ) {
  case OP_GREYSCALE:
    ret = IGreyscale ( image );
    break;
  case OP_NEGATE:
    ret = INegate ( image );
    break;
  case OP_BRIGHTNESS:
    ret = IBrightnessContrast ( image, op->i1, 0 );
    break;
  case OP_CONTRAST:
    ret = IBrightnessContrast ( image, 0, op->i1 );
    break;
  case OP_GAMMA:
    ret = IGamma ( image, op->d1 );
    break;
  case OP_THRESHOLD:
    ret = IThreshold ( image, (unsigned int) op->i1 );
    break;
  case OP_FLIP:
    ret = IFlip ( image );
    break;
  case OP_FLOP:
    ret = IFlop ( image );
    break;
  case OP_ROTATE:
    /* Exact (non-interpolating) for multiples of 90; otherwise a smooth
       arbitrary-angle rotation filled with the background colour. */
    if ( fmod ( op->d1, 90.0 ) == 0.0 ) {
      ret = IRotate ( image, (int) op->d1 );
    }
    else {
      IColor bg;
      /* IAllocNamedColor only reads the name; cast away const safely. */
      if ( IAllocNamedColor ( (char *) background, &bg ) != INoError )
        bg = IAllocColor ( 255, 255, 255 );
      ret = IRotateAngle ( image, op->d1, bg );
    }
    break;
  case OP_BLUR:
    ret = IBlur ( image, (unsigned int) op->i1 );
    break;
  case OP_GAUSSIAN_BLUR:
    ret = IGaussianBlur ( image, op->d1 );
    break;
  case OP_SHARPEN:
    ret = ISharpen ( image );
    break;
  case OP_EDGE:
    ret = IEdgeDetect ( image );
    break;
  case OP_EMBOSS:
    ret = IEmboss ( image );
    break;
  case OP_RESIZE:
    ret = IResizeFiltered ( image, (unsigned int) op->i1,
      (unsigned int) op->i2, IRESIZE_AUTO );
    break;
  case OP_REDUCE_COLORS:
    ret = IReduceColors ( image, (unsigned int) op->i1 );
    break;
  case OP_NORMALIZE:
    ret = INormalize ( image );
    break;
  case OP_SEPIA:
    ret = ISepia ( image );
    break;
  case OP_OPACITY:
    ret = IOpacity ( image, op->d1 );
    break;
  case OP_TRIM:
    ret = ITrim ( image, (unsigned int) op->i1 );
    break;
  case OP_BORDER: {
    IColor bg;
    if ( IAllocNamedColor ( (char *) background, &bg ) != INoError )
      bg = IAllocColor ( 255, 255, 255 );
    ret = IBorder ( image, (unsigned int) op->i1, bg );
    break;
  }
  }

  if ( ret != INoError ) {
    fprintf ( stderr, "Operation failed: %s\n", IErrorString ( ret ) );
    exit ( 1 );
  }
}

int main ( int argc, char *argv[] )
{
  IImage image;
  char *outfile = NULL;
  char *infile = NULL;
  const char *background = "white";
  IFileFormat input_format = IFORMAT_PPM;
  IFileFormat output_format = IFORMAT_PPM;
  Op ops[MAX_OPS];
  int nops = 0;
  FILE *fp;
  IError ret;
  int loop;
  int dither = 0;
  int auto_orient = 0;

  for ( loop = 1; loop < argc; loop++ ) {
    char *tok = argv[loop];
    Op op;
    memset ( &op, 0, sizeof ( op ) );

    if ( is_flag ( tok, "help" ) || is_flag ( tok, "h" ) ) {
      usage ( argv[0] );
      return ( 0 );
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
    else if ( is_flag ( tok, "background" ) ) {
      background = next_arg ( &loop, argc, argv, "--background" );
      continue; /* a setting, not a pipeline op */
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
    else if ( is_flag ( tok, "auto-orient" ) ) {
      auto_orient = 1; /* a read option, not a pipeline op */
      continue;
    }
    else if ( tok[0] == '-' && tok[1] != '\0' ) {
      fprintf ( stderr, "Unknown option: %s\n", tok );
      usage ( argv[0] );
      return ( 1 );
    }
    else if ( infile == NULL ) {
      infile = tok;
      continue;
    }
    else if ( outfile == NULL ) {
      outfile = tok;
      continue;
    }
    else {
      fprintf ( stderr, "Unexpected argument: %s\n", tok );
      return ( 1 );
    }

    if ( nops >= MAX_OPS ) {
      fprintf ( stderr, "Too many operations (max %d).\n", MAX_OPS );
      return ( 1 );
    }
    ops[nops++] = op;
  }

  if ( !infile )
    fprintf ( stderr, "No infile specified.  Reading from stdin.\n" );
  if ( !outfile ) {
    outfile = "out.ppm";
    fprintf ( stderr, "No outfile specified.  Writing to %s.\n", outfile );
  }

  /* Determine file types by extension. */
  if ( infile ) {
    ret = IFileType ( infile, &input_format );
    if ( ret ) {
      fprintf ( stderr, "Input file error: %s\n", IErrorString ( ret ) );
      exit ( 1 );
    }
  }
  ret = IFileType ( outfile, &output_format );
  if ( ret ) {
    fprintf ( stderr, "Output file error: %s\n", IErrorString ( ret ) );
    fprintf ( stderr, "Using PPM format.\n" );
    output_format = IFORMAT_PPM;
  }

  if ( infile ) {
    fp = fopen ( infile, "rb" );
    if ( !fp ) {
      perror ( "Error opening input file:" );
      exit ( 1 );
    }
  }
  else
    fp = stdin;

  if ( ( ret = IReadImageFile ( fp, input_format,
           auto_orient ? IOPTION_AUTOORIENT : IOPTION_NONE, &image ) ) ) {
    fprintf ( stderr, "Error reading image: %s\n", IErrorString ( ret ) );
    exit ( 1 );
  }
  if ( infile )
    fclose ( fp );

  /* Apply the operation pipeline in command-line order. */
  for ( loop = 0; loop < nops; loop++ )
    apply_op ( image, &ops[loop], background );

  if ( outfile ) {
    fp = fopen ( outfile, "wb" );
    if ( !fp ) {
      perror ( "Cannot open output file: " );
      exit ( 1 );
    }
  }
  else
    fp = stdout;

  ret = IWriteImageFile ( fp, image, output_format,
    IOPTION_INTERLACED | ( dither ? IOPTION_DITHER : 0 ) );
  if ( ret != INoError )
    fprintf ( stderr, "Error writing image: %s\n", IErrorString ( ret ) );

  if ( outfile )
    fclose ( fp );

  IFreeImage ( image );

  return ( ret == INoError ? 0 : 1 );
}
