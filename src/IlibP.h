/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Ilib.h
 *
 * Image library Private include
 *
 * Description:
 *	To be included only be Ilib internal functions
 *
 * History:
 *	15-Aug-01	Craig Knudsen	cknudsen@cknudsen.com
 *			Added more #ifdef to avoid compile errors if
 *			not using jpeg, png or gif.
 *	01-Apr-00	Jim Winstead	jimw@trainedmonkey.com
 *			Added _IReadBMP()
 *	26-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added _IReadXPM()
 *	23-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added text_style to IGCP.
 *	22-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added JPEG support.
 *	19-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added PNG support.
 *	19-May-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Added _ISetPointRGB macro
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#ifndef _ilibp_h
#define _ilibp_h

/*
** Define a magic value to place at the front of each structure.
** If this value is incorrect, then an invalid structure was
** passed in.
*/
#define IMAGIC_IMAGE 467
#define IMAGIC_GC 333
#define IMAGIC_COLOR 847
#define IMAGIC_FONT 104

#ifndef PI
#define PI 3.14159265358979323846
#endif /* PI */


/*
** Default comment for images
*/
#define IDEFAULT_COMMENT "Creator: Ilib http://ilib.sourceforge.net/"

/*
** Structures
*/
typedef struct {
  unsigned int magic;  /* memory verification */
  unsigned char red;   /* red value (0-255) */
  unsigned char green; /* green value (0-255) */
  unsigned char blue;  /* blue value (0-255) */
  unsigned char alpha; /* alpha (0=transparent .. 255=opaque) */
  unsigned long value; /* pixel value */
} IColorP;

typedef struct {
  unsigned int magic;        /* memory verification */
  int width;                 /* width */
  int height;                /* height */
  char *comments;            /* comments */
  unsigned char *data;       /* image data */
  unsigned short interlaced; /* interlaced output? */
  unsigned short greyscale;  /* greyscale? */
  unsigned short channels;   /* bytes per pixel: 1=grey, 3=RGB, 4=RGBA */
  unsigned short has_alpha;  /* convenience flag: channels == 4 */
  IColorP *transparent;      /* transparent color */
} IImageP;

#define IFONT_BDF 0 /* X11 BDF bitmap font (IFontBDF) */
#define IFONT_TTF 1 /* scalable font via FreeType (IFontTTF) */

typedef struct {
  unsigned int magic; /* memory verification */
  char *name;
  int type; /* IFONT_BDF (default) or IFONT_TTF */
} IFontP;

typedef struct {
  unsigned int magic;         /* memory verification */
  IColorP *foreground;        /* foreground color */
  IColorP *background;        /* background color */
  IFontP *font;               /* font */
  unsigned short antialiased; /* use anti-aliased *fonts* (size is halved) */
  unsigned short aa;          /* anti-alias drawing primitives (lines, ...) */
  unsigned int line_width;    /* line width */
  ILineStyle line_style;      /* line style */
  ITextStyle text_style;      /* line style */
  IBlendMode blend_mode;      /* how foreground is composited (default REPLACE) */
  /* add more later ... */
} IGCP;

/*
** File formats.
*/

IError _IWritePGM (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);

IError _IReadPPM (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);

IError _IWritePPM (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);

IError _IWriteXPM (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);

IError _IReadXPM (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);

#ifdef HAVE_PNGLIB
IError _IReadPNG (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);
#endif /* HAVE_PNGLIB */

#ifdef HAVE_PNGLIB
IError _IWritePNG (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);
#endif /* HAVE_PNGLIB */

#ifdef HAVE_GIFLIB
IError _IReadGIF (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);
#endif /* HAVE_GIFLIB */

#ifdef HAVE_GIFLIB
IError _IWriteGIF (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);
#endif /* HAVE_GIFLIB */

#ifdef HAVE_JPEGLIB
IError _IReadJPEG (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);
#endif /* HAVE_JPEGLIB */

#ifdef HAVE_JPEGLIB
IError _IWriteJPEG (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);
#endif /* HAVE_JPEGLIB */

IError _IReadBMP (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);

IError _IWriteBMP (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);

#ifdef HAVE_WEBPLIB
IError _IReadWEBP (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);

IError _IWriteWEBP (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);
#endif /* HAVE_WEBPLIB */

#ifdef HAVE_AVIFLIB
IError _IReadAVIF (
#ifndef _NO_PROTO
  FILE *fp,
  IOptions options,
  IImageP **image_return
#endif
);

IError _IWriteAVIF (
#ifndef _NO_PROTO
  FILE *fp,
  IImageP *image,
  IOptions options
#endif
);
#endif /* HAVE_AVIFLIB */

/* Median-cut color reduction. Writes <= max_colors distinct colors from src
   (npixels of RGB) into dst, which may alias src. */
void _IReduceColorsRGB (
#ifndef _NO_PROTO
  const unsigned char *src,
  int npixels,
  int max_colors,
  unsigned char *dst
#endif
);

/* Composite the GC foreground over pixel (x,y) using source-over, scaled by
   cover (0-255 edge coverage; 255 = fully covered). Bounds-checked. */
void _IBlendPoint (
#ifndef _NO_PROTO
  IImageP *image,
  IGCP *gc,
  int x,
  int y,
  unsigned int cover
#endif
);

typedef struct {
  char *name;
  IError ( *read_func ) (
#ifndef _NO_PROTO
    FILE *fp,              /* output file pointer */
    IOptions options,      /* read options */
    IImageP **image_return /* returned image */
#endif
  );
  IError ( *write_func ) (
#ifndef _NO_PROTO
    FILE *fp,        /* output file pointer */
    IImageP *image,  /* image to save */
    IOptions options /* write options */
#endif
  );
  int maxdepth; /* max depth (1=b/w, 8=256 colors) */
  int grey;     /* is grey? */
  int alpha_ok; /* writer accepts an RGBA image directly (no flatten) */
} IFormatDef;
#ifdef IIncludeFileFormats
static IFormatDef IFileFormats[] = {
#ifdef HAVE_GIFLIB
  { /* IFORMAT_GIF */ "GIF", _IReadGIF, _IWriteGIF, 8, 0, 0 },
#else
  { /* IFORMAT_GIF */ "GIF", NULL, NULL, 8, 0, 0 },
#endif
  { /* IFORMAT_PPM */ "PPM", _IReadPPM, _IWritePPM, 24, 0, 0 },
  { /* IFORMAT_PGM */ "PGM", _IReadPPM, _IWritePGM, 8, 1, 0 },
  { /* IFORMAT_PBM */ "PBM", NULL, NULL, 1, 1, 0 },
  { /* IFORMAT_XPM */ "XPM", _IReadXPM, _IWriteXPM, 24, 0, 0 },
  { /* IFORMAT_XBM */ "XBM", NULL, NULL, 1, 1, 0 },
#ifdef HAVE_PNGLIB
  { /* IFORMAT_PNG */ "PNG", _IReadPNG, _IWritePNG, 24, 0, 1 },
#else
  { /* IFORMAT_PNG */ "PNG", NULL, NULL, 24, 0, 0 },
#endif
#ifdef HAVE_JPEGLIB
  { /* IFORMAT_JPEG */ "JPEG", _IReadJPEG, _IWriteJPEG, 24, 0, 0 },
#else
  { /* IFORMAT_JPEG */ "JPEG", NULL, NULL, 24, 0, 0 },
#endif
  { /* IFORMAT_BMP */ "BMP", _IReadBMP, _IWriteBMP, 24, 0, 0 },
#ifdef HAVE_WEBPLIB
  { /* IFORMAT_WEBP */ "WEBP", _IReadWEBP, _IWriteWEBP, 24, 0, 1 },
#else
  { /* IFORMAT_WEBP */ "WEBP", NULL, NULL, 24, 0, 1 },
#endif
#ifdef HAVE_AVIFLIB
  { /* IFORMAT_AVIF */ "AVIF", _IReadAVIF, _IWriteAVIF, 24, 0, 1 },
#else
  { /* IFORMAT_AVIF */ "AVIF", NULL, NULL, 24, 0, 1 },
#endif
};
#endif /* IIncludeFileFormats */

IColorP *_IGetColor (
#ifndef _NO_PROTO
  int color /* int index to translate */
#endif
);

IError _IFontBDFGetSize (
#ifndef _NO_PROTO
  char *name,                 /* font name */
  unsigned int *height_return /* out: height of font */
#endif
);

#ifdef HAVE_FREETYPE
/* Scalable (TrueType/OpenType) fonts via FreeType. The face is kept in a
   private registry keyed by name, mirroring the BDF backend. */
IError _IFontTTFLoad (
#ifndef _NO_PROTO
  char *name,    /* registry name */
  char *path,    /* path to .ttf/.otf file */
  int pixel_size /* nominal pixel height */
#endif
);

IError _IFontTTFGetSize (
#ifndef _NO_PROTO
  char *name,
  unsigned int *height_return
#endif
);

IError _IFontTTFDrawString (
#ifndef _NO_PROTO
  IImageP *image,
  IGCP *gc,
  int x,
  int y,
  char *text,
  unsigned int len,
  double angle /* degrees; 0 = horizontal left-to-right */
#endif
);

void _IFontTTFFree (
#ifndef _NO_PROTO
  char *name
#endif
);
#endif /* HAVE_FREETYPE */

/*
** Define a function to draw a point.  Must be inline because this
** is typically within multiple for loops and we don't want all those
** extra stack pushes and pops slowing us down...
** Arguments are ( ImageP *, IGCP *, int, int ).
*/
#define _IDrawPoint _ISetPoint
/* Write the GC foreground at (x,y). With IBLEND_OVER the foreground is
   composited (source-over) via _IBlendPoint; otherwise it overwrites the
   pixel. The replace path keeps separate literal-stride branches per channel
   count for speed. */
#define _ISetPoint( i, g, x, y )                                          \
  {                                                                       \
    if ( ( g )->blend_mode == IBLEND_OVER ) {                             \
      _IBlendPoint ( ( i ), ( g ), ( x ), ( y ), 255 );                   \
    }                                                                     \
    else {                                                                \
      unsigned char *ptrX;                                                \
      if ( ( x ) >= 0 && ( x ) < ( i )->width && ( y ) >= 0 &&            \
           ( y ) < ( i )->height ) {                                      \
        if ( ( i )->greyscale ) {                                         \
          ptrX = ( i )->data + ( ( y ) * ( i )->width ) + ( x );          \
          *ptrX = ( g )->foreground->red;                                 \
        }                                                                 \
        else if ( ( i )->channels == 4 ) {                                \
          ptrX = ( i )->data + ( ( y ) * ( i )->width + ( x ) ) * 4;      \
          *ptrX = ( g )->foreground->red;                                 \
          *( ptrX + 1 ) = ( g )->foreground->green;                       \
          *( ptrX + 2 ) = ( g )->foreground->blue;                        \
          *( ptrX + 3 ) = ( g )->foreground->alpha;                       \
        }                                                                 \
        else {                                                            \
          ptrX = ( i )->data + ( ( y ) * ( i )->width * 3 ) + ( (x) *3 ); \
          *ptrX = ( g )->foreground->red;                                 \
          *( ptrX + 1 ) = ( g )->foreground->green;                       \
          *( ptrX + 2 ) = ( g )->foreground->blue;                        \
        }                                                                 \
      }                                                                   \
    }                                                                     \
  }

#define _ISetPointRGB( i, x, y, r, g, b )                                              \
  {                                                                                    \
    unsigned char *ptrX;                                                               \
    if ( ( x ) >= 0 && ( x ) < ( i )->width && ( y ) >= 0 && ( y ) < ( i )->height ) { \
      if ( ( i )->greyscale ) {                                                        \
        ptrX = ( i )->data + ( ( y ) * ( i )->width ) + ( x );                         \
        *ptrX = ( r );                                                                 \
      }                                                                                \
      else if ( ( i )->channels == 4 ) {                                               \
        ptrX = ( i )->data + ( ( y ) * ( i )->width + ( x ) ) * 4;                     \
        *ptrX = ( r );                                                                 \
        *( ptrX + 1 ) = ( g );                                                         \
        *( ptrX + 2 ) = ( b );                                                         \
        *( ptrX + 3 ) = 255;                                                           \
      }                                                                                \
      else {                                                                           \
        ptrX = ( i )->data + ( ( y ) * ( i )->width * 3 ) + ( (x) *3 );                \
        *ptrX = ( r );                                                                 \
        *( ptrX + 1 ) = ( g );                                                         \
        *( ptrX + 2 ) = ( b );                                                         \
      }                                                                                \
    }                                                                                  \
  }

#define _IGetPointColor( i, x, y, c )                                                  \
  {                                                                                    \
    unsigned char *ptrX;                                                               \
    if ( ( x ) >= 0 && ( x ) < ( i )->width && ( y ) >= 0 && ( y ) < ( i )->height ) { \
      if ( ( i )->greyscale ) {                                                        \
        ptrX = ( i )->data + ( ( y ) * ( i )->width ) + ( x );                         \
        ( c ).red = ( c ).green = ( c ).blue = *ptrX;                                  \
        ( c ).alpha = 255;                                                             \
      }                                                                                \
      else if ( ( i )->channels == 4 ) {                                               \
        ptrX = ( i )->data + ( ( y ) * ( i )->width + ( x ) ) * 4;                     \
        ( c ).red = *ptrX;                                                             \
        ( c ).green = *( ptrX + 1 );                                                   \
        ( c ).blue = *( ptrX + 2 );                                                    \
        ( c ).alpha = *( ptrX + 3 );                                                   \
      }                                                                                \
      else {                                                                           \
        ptrX = ( i )->data + ( ( y ) * ( i )->width * 3 ) + ( (x) *3 );                \
        ( c ).red = *ptrX;                                                             \
        ( c ).green = *( ptrX + 1 );                                                   \
        ( c ).blue = *( ptrX + 2 );                                                    \
        ( c ).alpha = 255;                                                             \
      }                                                                                \
    }                                                                                  \
  }

#define _IColorsMatch( c1, c2 ) \
  ( ( c1 ).red == ( c2 ).red && ( c1 ).green == ( c2 ).green && ( c1 ).blue == ( c2 ).blue )

#endif /* _ilibp_h */
