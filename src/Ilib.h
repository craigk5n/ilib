/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Ilib.h
 *
 * Image library
 *
 * Description:
 *	The Ilib library is a portable set of routines for manipulating
 *	images.  It has been successfully compiled and run on various
 *	UNIX platforms as well as Windows 95/98/NT
 *	(gcc, lcc-win32 and MS Visual C++).
 *	The source is 32-bit, so this will never be a 16-bit Windows
 *	application.  It is intended to be independent of any
 *	windowing system or graphics file format.
 *
 * History:
 *	25-Oct-04	Added IFloodFill()
 *			Changed version to 1.1.9
 *	15-Aug-01	Changed version to 1.1.8
 *	23-May-00	Changed version to 1.1.7
 *	01-Apr-00	Jim Winstead	jimw@trainedmonkey.com
 *			Add BMP support (read-only).
 *	21-Jan-00	Geovan Rodriguez <geovan@cigb.edu.cu>
 *			Added IDrawStringRotatedAngle()
 *	24-Nov-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IDrawArc(), IDrawEllipse(), IDrawCircle(),
 *			IDrawPolygon(), IFillPolygon(),
 *			IFillArc(), IFillCircle(), IFillEllipse(),
 *			IDrawEnclosedArc(), IArcProperties()
 *			Added IPoint structure
 *	26-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IAllocNamedColor()
 *	25-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Changed version to 1.1.5
 *	23-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added ISetTextStyle().
 *	21-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added IDrawStringRotated()
 *	20-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Changed version to 1.1.4
 *			Added IGetTransparent()
 *	19-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added ISetComment()
 *			Fixed GIF to write interlaced output.
 *			Fixed GIF to read and write comments.
 *	18-Aug-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Updated GIF support to handle reading interlaced
 *			GIFs (and ignore any other GIF extensions).
 *	22-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added JPEG support.
 *			Added PGM support.
 *			Added ICopyImageScaled()
 *			Changed version to 1.1.3
 *	19-Jul-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Added PNG support.
 *			Changed version to 1.1.2
 *	12-Apr-99	Craig Knudsen	cknudsen@cknudsen.com
 *			Changed version to 1.1.1
 *	17-May-98	Craig Knudsen	cknudsen@cknudsen.com
 *			Changed definition of IFreeColor
 *			and added some version definitions
 *			Added ITextHeight() and ITextDimensions().
 *			Added IFileType().
 *			Added IErrorString().
 *			Moved file type definitions into an enum.
 *			Added ability to read GIFs (thanks to GIFLIB).
 *	20-May-96	Craig Knudsen	cknudsen@cknudsen.com
 *			Created
 *
 ****************************************************************************/

#ifndef _ilib_h
#define _ilib_h

/**
 * @file Ilib.h
 * @brief Public API for Ilib, a C raster-image library.
 *
 * @mainpage Ilib
 *
 * Ilib is a C library for reading, creating, manipulating and saving raster
 * images. It can draw text using X11 BDF fonts and read/write PPM, PGM, XPM,
 * XBM, GIF, PNG and JPEG (and read BMP). The drawing API is modeled on a
 * subset of the X11 graphics functions.
 *
 * @section handles Opaque handles
 * The public types ::IImage, ::IFont, ::IGC and ::IColor are opaque. Construct
 * them with ICreateImage(), ICreateGC(), ILoadFontFromFile() / IAllocColor(),
 * and release them with the IFreeImage(), IFreeGC(), IFreeFont() and
 * IFreeColor() macros.
 *
 * @section errors Error handling
 * Most functions return an ::IError code (::INoError is success);
 * IErrorString() maps a code to a message. Constructor functions return a
 * handle or NULL/0 on failure.
 *
 * @section start Getting started
 * See the bundled `examples/` (iconvert, isample) and the project README for
 * build and usage instructions.
 */

#include <stdio.h> /* for FILE * in the read/write prototypes */

#define ILIB_MAJOR_VERSION 1
#define ILIB_MINOR_VERSION 1
#define ILIB_MICRO_VERSION 10

#define ILIB_VERSION "1.1.10"
#define ILIB_VERSION_DATE "25 Oct 2004"
#define ILIB_URL "https://www.k5n.us/Ilib.php"


/**
 * Structures
 */
typedef void *IImage;          /* image */
typedef void *IFont;           /* font */
typedef void *IGC;             /* graphics context */
typedef unsigned int IColor;   /* color */
typedef unsigned int IOptions; /* options */

typedef enum {
  IFORMAT_GIF = 0,
  IFORMAT_PPM = 1,
  IFORMAT_PGM = 2,
  IFORMAT_PBM = 3,
  IFORMAT_XPM = 4,
  IFORMAT_XBM = 5,
  IFORMAT_PNG = 6,
  IFORMAT_JPEG = 7,
  IFORMAT_BMP = 8
} IFileFormat;
#define INUM_FORMATS 9

/**
 * Line drawing styles.
 * Applies to: IDrawLine, IDrawRectangle
 */
typedef enum {
  ILINE_SOLID,       /* default */
  ILINE_ON_OFF_DASH, /* dashes (every 3 pixels) */
  ILINE_DOUBLE_DASH  /* not yet implemented */
} ILineStyle;

/**
 * Fill styles.
 * Applies to: nothing yet (not implemented)
 */
typedef enum {
  IFILL_SOLID,          /* default */
  IFILL_TILED,          /* not yet implemented */
  IFILL_STIPPLED,       /* not yet implemented */
  IFILL_OPAQUE_STIPPLED /* not yet implemented */
} IFillStyle;

/**
 * Text drawing styles.
 * Applies to: IDrawString, IDrawStringRotated
 */
typedef enum {
  ITEXT_NORMAL,     /* default */
  ITEXT_ETCHED_IN,  /* text appears etched into background */
  ITEXT_ETCHED_OUT, /* text appears etched out of background */
  ITEXT_SHADOWED    /* text has shadow that fades into background */
} ITextStyle;

/**
 * Text drawing directions
 * Applies to: IDrawStringRotated()
 */
typedef enum {
  ITEXT_LEFT_TO_RIGHT, /* default */
  ITEXT_BOTTOM_TO_TOP,
  ITEXT_TOP_TO_BOTTOM
} ITextDirection;

/**
 * Pixel compositing modes (set on a graphics context with ISetBlendMode).
 */
typedef enum {
  IBLEND_REPLACE = 0, /* overwrite the destination (default) */
  IBLEND_OVER         /* source-over alpha compositing */
} IBlendMode;

/**
 * Defines a structure for specifying a point.
 */
typedef struct {
  int x;
  int y;
} IPoint;


/**
 * Options
 */
#define IOPTION_NONE 0x0000

/* use the following with ICreateImage() */
#define IOPTION_GREYSCALE 0x0001 /* greyscale image */
#define IOPTION_GRAYSCALE IOPTION_GREYSCALE
#define IOPTION_ALPHA 0x0004 /* RGBA image (4 channels) */

/* use the following with IWriteImageFile() */
#define IOPTION_ASCII 0x0001      /* ascii output for pbm/pgm/ppm */
#define IOPTION_INTERLACED 0x0002 /* interlaced output (GIF) */

/**
 * Default color values for black and white
 */
#define IBLACK_PIXEL 0
#define IWHITE_PIXEL 1


/**
 * Errors
 */
typedef enum {
  INoError = 0,
  IInvalidImage,
  IInvalidGC,
  IInvalidColor,
  INoTransparentColor,
  IInvalidFont,
  IFunctionNotImplemented,
  IInvalidFormat,
  IFileInvalid,
  IErrorWriting,
  INoFontSet,
  INoSuchFont,
  INoSuchFile,
  IFontError,
  IInvalidEscapeSequence,
  IInvalidArgument,
  IInvalidColorName,
  IGIFError,
  INoGIFSupport,
  IPNGError,
  INoPNGSupport,
  IInvalidPolygon
} IError;


/**
 * Functions
 */


/**
 * Convert an IError value into text suitable for printing in an error
 * message.
 */
char *IErrorString (
#ifndef _NO_PROTO
  IError err /* error value */
#endif
);

/**
 * Create a blank (white) image of a specified width and height.
 */
IImage ICreateImage (
#ifndef _NO_PROTO
  unsigned int width,  /* image width */
  unsigned int height, /* image height */
  unsigned int options /* options flags (IOPTION_GREYSCALE, etc.) */
#endif
);

/**
 * Creates a duplicate of the original image.
 */
IError IDuplicateImage (
#ifndef _NO_PROTO
  IImage image,        /* image to duplicate */
  IImage *image_return /* out: pointer to new image */
#endif
);

/**
 * Copies all or part of one image onto another image at a specified
 * coordinate.
 */
IError ICopyImage (
#ifndef _NO_PROTO
  IImage source,       /* source image */
  IImage dest,         /* destination image */
  IGC gc,              /* graphics context */
  int src_x,           /* x from source image */
  int src_y,           /* y from source image */
  unsigned int width,  /* width to copy */
  unsigned int height, /* height to copy */
  int dest_x,          /* x coordinate on the destination image */
  int dest_y           /* y coordinate on the destination image */
#endif
);

/**
 * Copies all of an image to another image by scaling it to fit.
 */
IError ICopyImageScaled (
#ifndef _NO_PROTO
  IImage source,           /* source image */
  IImage dest,             /* destination image */
  IGC gc,                  /* graphics context */
  int src_x,               /* x from source image */
  int src_y,               /* y from source image */
  unsigned int src_width,  /* width of source image to copy */
  unsigned int src_height, /* height of source image to copy */
  int dest_x,              /* x coordinate on the destination image */
  int dest_y,              /* y coordinate on the destination image */
  unsigned int dest_width, /* width to copy to */
  unsigned int dest_height /* height to copy to */
#endif
);

/**
 * Reduce the number of distinct colors in an image to at most max_colors,
 * using median-cut quantization. This is mainly useful before writing GIF,
 * which is limited to 256 colors (the GIF writer calls it automatically).
 * Greyscale images, and images that already have at most max_colors colors,
 * are left unchanged. max_colors is clamped to 256.
 */
IError IReduceColors (
#ifndef _NO_PROTO
  IImage image,           /* image to reduce (modified in place) */
  unsigned int max_colors /* maximum number of colors to keep (<= 256) */
#endif
);


#define IFreeImage( i ) \
  _IFreeImage ( i );    \
  ( i ) = NULL;

/**
 * Frees the memory associated with an image that will not be used again.
 */
IError _IFreeImage (
#ifndef _NO_PROTO
  IImage image /* pointer to image */
#endif
);

/**
 * Returns the height of the image.
 */
unsigned int IImageHeight (
#ifndef _NO_PROTO
  IImage image /* pointer to image */
#endif
);

/**
 * Returns the width of the image.
 */
unsigned int IImageWidth (
#ifndef _NO_PROTO
  IImage image /* pointer to image */
#endif
);

/**
 * Set the color of a single pixel (RGB, 0-255 per channel). For a greyscale
 * image the red value is stored. Returns IInvalidArgument if (x,y) is outside
 * the image or any channel exceeds 255.
 */
IError ISetPixel (
#ifndef _NO_PROTO
  IImage image,       /* image */
  int x,              /* x coordinate */
  int y,              /* y coordinate */
  unsigned int red,   /* red value (0-255) */
  unsigned int green, /* green value (0-255) */
  unsigned int blue   /* blue value (0-255) */
#endif
);

/**
 * Get the color of a single pixel. For a greyscale image all three channels
 * return the stored value. Any of the return pointers may be NULL. Returns
 * IInvalidArgument if (x,y) is outside the image.
 */
IError IGetPixel (
#ifndef _NO_PROTO
  IImage image,               /* image */
  int x,                      /* x coordinate */
  int y,                      /* y coordinate */
  unsigned int *red_return,   /* out: red value (or NULL) */
  unsigned int *green_return, /* out: green value (or NULL) */
  unsigned int *blue_return   /* out: blue value (or NULL) */
#endif
);

/**
 * Set the color of a single pixel including alpha. On a non-alpha (RGB or
 * greyscale) image the alpha is ignored. Returns IInvalidArgument if (x,y) is
 * outside the image or any channel exceeds 255.
 */
IError ISetPixelAlpha (
#ifndef _NO_PROTO
  IImage image,       /* image */
  int x,              /* x coordinate */
  int y,              /* y coordinate */
  unsigned int red,   /* red value (0-255) */
  unsigned int green, /* green value (0-255) */
  unsigned int blue,  /* blue value (0-255) */
  unsigned int alpha  /* alpha value (0-255) */
#endif
);

/**
 * Get the color of a single pixel including alpha. For non-alpha images the
 * returned alpha is 255 (opaque). Any of the return pointers may be NULL.
 */
IError IGetPixelAlpha (
#ifndef _NO_PROTO
  IImage image,               /* image */
  int x,                      /* x coordinate */
  int y,                      /* y coordinate */
  unsigned int *red_return,   /* out: red (or NULL) */
  unsigned int *green_return, /* out: green (or NULL) */
  unsigned int *blue_return,  /* out: blue (or NULL) */
  unsigned int *alpha_return  /* out: alpha (or NULL) */
#endif
);

/**
 * Set the comment.
 */
IError ISetComment (
#ifndef _NO_PROTO
  IImage image, /* pointer to image */
  char *comment /* new comment */
#endif
);

/**
 * Get the comment (if there is one).
 */
IError IGetComment (
#ifndef _NO_PROTO
  IImage image,  /* pointer to image */
  char **comment /* address to comment */
#endif
);

/**
 * Determine the type of image by the file extension.
 */
IError IFileType (
#ifndef _NO_PROTO
  char *file,                /* filename */
  IFileFormat *format_return /* out: format of file */
#endif
);

/**
 * Creates an image from an image file.
 * Currently can read raw PPM (IFORMAT_PPM), XPM (IFORMAT_XPM),
 * GIF (IFORMAT_GIF), PNG (IFORMAT_PNG), and JPEG (IFORMAT_JPEG) files.
 * The file is left open for the caller to close.
 * (Note: you should set the file to binary mode on Win32 platforms
 * by using something like: <BR>
 * <TT>&nbsp;&nbsp;
 * file = fopen ( filename, "rb" );
 * </TT>
 */
IError IReadImageFile (
#ifndef _NO_PROTO
  FILE *fp,            /* file pointer */
  IFileFormat format,  /* output format (e.g. IFORMAT_GIF) */
  IOptions options,    /* options */
  IImage *image_return /* out: returned image on success */
#endif
);

/**
 * Writes an image to a file.
 * Currently supports writing PPM (IFORMAT_PPM), XPM (IFORMAT_XPM),
 * GIF (IFORMAT_GIF), PNG (IFORMAT_PNG) and JPEG (IFORMAT_JPEG) formats.
 * The file is left open for the caller to close.
 * (Note: you should set the file to binary mode on Win32 platforms.)
 */
IError IWriteImageFile (
#ifndef _NO_PROTO
  FILE *fp,           /* file pointer */
  IImage image,       /* image */
  IFileFormat format, /* output format (e.g. IFORMAT_XPM) */
  IOptions options    /* options (e.g. IOPTION_INTERLACED) */
#endif
);

/**
 * Creates a graphic context for drawing on an image.
 */
IGC ICreateGC ();

#define IFreeGC( g ) \
  _IFreeGC ( g );    \
  ( g ) = NULL;

/**
 * Frees a graphic context no longer in use.
 */
IError _IFreeGC (
#ifndef _NO_PROTO
  IGC gc /* graphics context */
#endif
);

/**
 * Load a font from a file. Currently only X11 BDF font files are supported.
 * A few sample fonts ship with Ilib (installed under
 * `<prefix>/share/ilib/fonts`); any X11 BDF font will also work. BDF is part
 * of the X11 Window System and the format is widely available.
 */
IError ILoadFontFromFile (
#ifndef _NO_PROTO
  char *name,        /* name to use for reference (filename) */
  char *path,        /* path to font file */
  IFont *font_return /* out: returned font */
#endif
);

/**
 * Load a scalable TrueType/OpenType font at a given pixel size, using FreeType.
 * Text drawn with this font (via ISetFont() + IDrawString()) is anti-aliased.
 * Returns IFunctionNotImplemented if the library was built without FreeType.
 * (Text styles and rotation are not yet applied to scalable-font text.)
 */
IError ILoadFontFromFileTTF (
#ifndef _NO_PROTO
  char *name,              /* name to use for reference */
  char *path,              /* path to .ttf / .otf file */
  unsigned int pixel_size, /* nominal glyph height in pixels */
  IFont *font_return       /* out: returned font */
#endif
);

/**
 * Loads a font from data passed in.  This is identical to
 * ILoadFontFromFile() except the
 * font data is passed in as an argument rather than a file.
 * This allows the font to be embedded in the application rather
 * than distributed as a separate file.
 */
IError ILoadFontFromData (
#ifndef _NO_PROTO
  char *name,   /* name to use for reference (filename) */
  char **lines, /* font data (identical to file contents) */
  /* as an array of character strings */
  /* (one text line per array element */
  /* terminated with a NULL element */
  IFont *font_return /* out: pointer to returned font */
#endif
);

#define IFreeFont( f ) \
  _IFreeFont ( f );    \
  ( f ) = NULL;

/**
 * Frees a font no longer in use.
 */
IError _IFreeFont (
#ifndef _NO_PROTO
  IFont font /* font to free */
#endif
);

/**
 * Gets the pixel height of the font.
 * (See ITextHeight() for calculating the
 * height of a multiline text.)
 */
IError IFontSize (
#ifndef _NO_PROTO
  IFont font,                 /* font */
  unsigned int *height_return /* out: height in pixels of font */
#endif
);

/**
 * Determines the length (in pixels) of the given text for
 * the specified font.
 */
IError ITextWidth (
#ifndef _NO_PROTO
  IGC gc,                    /* graphics context */
  IFont font,                /* font */
  char *text,                /* text */
  unsigned int len,          /* length of text */
  unsigned int *width_return /* out: width in pixels of text */
#endif
);


/**
 * Determines the height (in pixels) of the given text for
 * the specified font. This is useful for multiline text.
 */
IError ITextHeight (
#ifndef _NO_PROTO
  IGC gc,                     /* graphics context */
  IFont font,                 /* font */
  char *text,                 /* text */
  unsigned int len,           /* length of text */
  unsigned int *height_return /* out: height in pixels of text */
#endif
);

/**
 * Determines the width & height (in pixels) of the given text for
 * the specified font. (useful for multiline text)
 */
IError ITextDimensions (
#ifndef _NO_PROTO
  IGC gc,                     /* graphics context */
  IFont font,                 /* font */
  char *text,                 /* text */
  unsigned int len,           /* length of text */
  unsigned int *width_return, /* out: width in pixels of text */
  unsigned int *height_return /* out: height in pixels of text */
#endif
);


/**
 * Determines the x-y coordinates of the starting, stopping and middle
 * point of an arc.  Any of the pointers may be NULL if you're
 * not interested in obtaining the value.
 */
IError IArcProperties (
#ifndef _NO_PROTO
  IGC gc,        /* graphics context */
  int x,         /* arc center x coordinate */
  int y,         /* arc center y coordinate */
  int r1,        /* angle 1 radius */
  int r2,        /* angle 2 radius */
  double a1,     /* first angle (in degrees) */
  double a2,     /* second angle (in degrees) */
  int *a1_x,     /* out: x coordinate of edge of angle 1 */
  int *a1_y,     /* out: y coordinate of edge of angle 1 */
  int *a2_x,     /* out: x coordinate of edge of angle 2 */
  int *a2_y,     /* out: y coordinate of edge of angle 2 */
  int *middle_x, /* out: x coordinate of edge center */
  int *middle_y  /* out: y coordinate of edge center */
#endif
);


/**
 * Allocates a color to be used for drawing.
 * (See ISetForeground()).
 */
IColor IAllocColor (
#ifndef _NO_PROTO
  unsigned int red,   /* red value (0-255) */
  unsigned int green, /* green value (0-255) */
  unsigned int blue   /* blue value (0-255) */
#endif
);

/**
 * Allocates a translucent color. Like IAllocColor() but with an alpha value
 * (0 = transparent .. 255 = opaque). Used with ISetForeground() and the
 * IBLEND_OVER blend mode (see ISetBlendMode()).
 */
IColor IAllocColorAlpha (
#ifndef _NO_PROTO
  unsigned int red,   /* red value (0-255) */
  unsigned int green, /* green value (0-255) */
  unsigned int blue,  /* blue value (0-255) */
  unsigned int alpha  /* alpha value (0-255) */
#endif
);

/**
 * Allocates a color by name (i.e. "blue") to be used for drawing.
 * (See ISetForeground()).
 */
IError IAllocNamedColor (
#ifndef _NO_PROTO
  char *colorname,  /* color name ("red, "orange", etc.) */
  IColor *color_ret /* resulting color */
#endif
);

#define IFreeColor( c ) \
  _IFreeColor ( c );    \
  ( c ) = 0;

/**
 * Frees a color no longer in use.
 */
IError _IFreeColor (
#ifndef _NO_PROTO
  IColor color /* color */
#endif
);

/**
 * Set the transparent color of the image.
 * This is only relevant to images that are written to either
 * GIF or XPM formats.
 */
IError ISetTransparent (
#ifndef _NO_PROTO
  IImage image, /* image */
  IColor color  /* color */
#endif
);

/**
 * Get the transparent color of the image.
 * If there is no transparent image set, then INoTransparentColor
 * is returned.
 */
IError IGetTransparent (
#ifndef _NO_PROTO
  IImage image, /* image */
  IColor *color /* returned color */
#endif
);


/**
 * Set the foreground color of a graphics context.
 * This will set the drawing color for drawing functions
 * (IDrawLine(),
 * IFillRectangle(), etc.)
 */
IError ISetForeground (
#ifndef _NO_PROTO
  IGC gc,      /* graphics context */
  IColor color /* color */
#endif
);

/**
 * Set the pixel compositing mode of a graphics context. IBLEND_REPLACE (the
 * default) overwrites pixels; IBLEND_OVER composites the foreground using its
 * alpha (source-over). Affects IDrawPoint() (and, in later phases, the other
 * primitives).
 */
IError ISetBlendMode (
#ifndef _NO_PROTO
  IGC gc,         /* graphics context */
  IBlendMode mode /* IBLEND_REPLACE or IBLEND_OVER */
#endif
);

/**
 * Enable or disable anti-aliasing of drawing primitives on a graphics context
 * (off by default). When on, IDrawLine() renders smooth (coverage-blended)
 * edges for thin solid lines; thick or dashed lines fall back to the aliased
 * rasterizer. Anti-aliasing composites edge coverage regardless of the blend
 * mode. (This is independent of ISetAntiAliasedFont(), which affects text.)
 */
IError ISetAntiAlias (
#ifndef _NO_PROTO
  IGC gc, /* graphics context */
  int on  /* non-zero to enable */
#endif
);

/**
 * Set the background color of a graphics context.
 * This will affect colors when using text styles.
 */
IError ISetBackground (
#ifndef _NO_PROTO
  IGC gc,      /* graphics context */
  IColor color /* color */
#endif
);

/**
 * Sets the current drawing font for a graphics context.
 * Subsequent calls to IDrawString()
 * will use the specified font.
 */
IError ISetFont (
#ifndef _NO_PROTO
  IGC gc,    /* graphics context */
  IFont font /* font */
#endif
);

/**
 * Sets the line drawing with for a graphics context.
 * (This applies to IDrawLine() and
 * IDrawRectangle().)
 */
IError ISetLineWidth (
#ifndef _NO_PROTO
  IGC gc,                 /* graphics context */
  unsigned int line_width /* line width */
#endif
);

/**
 * Sets the current line drawing style for a graphics context.
 * Currently support ILINE_SOLID (default)
 * and ILINE_ON_OFF_DASH.  (This applies to
 * IDrawLine() and
 * IDrawRectangle().)
 */
IError ISetLineStyle (
#ifndef _NO_PROTO
  IGC gc,               /* graphics context */
  ILineStyle line_style /* line style */
#endif
);

/**
 * Sets the curret text drawing style for a graphics context.
 * Supported styles are:<UL>
 * <LI> ITEXT_NORMAL - default (no effects)
 * <LI> ITEXT_ETCHED_IN - draw text etched into background (does not
 *      use foreground color, just background)
 * <LI> ITEXT_ETCHED_OUT - draw text etched out of background (does not
 *      use foreground color, just background)
 * <LI> ITEXT_SHADOWED - draw text with a dark shadow to the lower left
 *      of the text.
 * </UL>
 * (This applies to
 * IDrawString() and
 * IDrawStringRotated().)
 */
IError ISetTextStyle (
#ifndef _NO_PROTO
  IGC gc,               /* graphics context */
  ITextStyle text_style /* text style */
#endif
);


/**
 * Draws text onto the image at the specified coordinate.
 * See the font definition file to determine which characters are supported.
 * <BR>Note: the font is set with
 * ISetFont().
 */
IError IDrawString (
#ifndef _NO_PROTO
  IImage image,    /* image */
  IGC gc,          /* graphics context */
  int x,           /* x coordinate (lower left of text) */
  int y,           /* y coordinate (lower left of text) */
  char *text,      /* pointer to text */
  unsigned int len /* length of text */
#endif
);

/**
 * Draws text onto the image at the specified coordinate using one
 * of the angles: ITEXT_LEFT_TO_RIGHT, ITEXT_TOP_TO_BOTTOM, or
 * ITEXT_BOTTOM_TO_TOP.
 * See the font definition file to determine which characters are supported.
 * <BR>Note: the font is set with
 * ISetFont().
 * The x-y coordinates always specify the starting point to draw from
 * (the lower left spot of the first character).
 * For example, for ITEXT_TOP_TO_BOTTOM the x-y would represent the
 * upper left area of the box where text will be drawn.
 */
IError IDrawStringRotated (
#ifndef _NO_PROTO
  IImage image,            /* image */
  IGC gc,                  /* graphics context */
  int x,                   /* x coordinate */
  int y,                   /* y coordinate */
  char *text,              /* pointer to text */
  unsigned int len,        /* length of text */
  ITextDirection direction /* direction to draw text */
#endif
);

/**
 * Draws text onto the image at the specified coordinate using one
 * the specified angle.  This is slightly slower than
 * IDrawStringRotated(), so only
 * use this function if you're drawing something other than
 * 90, 180, or 270 degrees.
 * See the font definition file to determine which characters are supported.
 * <BR>Note: the font is set with
 * ISetFont().
 * The x-y coordinates always specify the starting point to draw from
 * (the lower left spot of the first character).
 */
IError IDrawStringRotatedAngle (
#ifndef _NO_PROTO
  IImage image,     /* image */
  IGC gc,           /* graphics context */
  int x,            /* x coordinate */
  int y,            /* y coordinate */
  char *text,       /* pointer to text */
  unsigned int len, /* length of text */
  double angle      /* angle to rotate text */
#endif
);


/**
 * Draws a single point on the image.
 */
IError IDrawPoint (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* x coordinate */
  int y         /* y coordinate */
#endif
);

/**
 * Draws a line onto the image using the graphics context's
 * current line style (
 * see ISetLineStyle()).
 */
IError IDrawLine (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x1,       /* 1st coordinate */
  int y1,       /* 1st coordinate */
  int x2,       /* 2nd coordinate */
  int y2        /* 2nd coordinate */
#endif
);

/**
 * Draws a polygon onto the image using the graphics context's
 * current line style (
 * see ISetLineStyle()).
 */
IError IDrawPolygon (
#ifndef _NO_PROTO
  IImage image,   /* image */
  IGC gc,         /* graphics context */
  IPoint *points, /* array of points */
  int npoints     /* size of above array */
#endif
);

/**
 * Draw a chain of cubic Bezier curves. points[0] is the start point; each
 * following group of three points is (control1, control2, end) of one cubic
 * segment, so npoints must be 1 + 3k (4, 7, 10, ...). The curve honors the GC
 * line style and anti-aliasing.
 */
IError IDrawBezier (
#ifndef _NO_PROTO
  IImage image,   /* image */
  IGC gc,         /* graphics context */
  IPoint *points, /* start point followed by (c1, c2, end) triples */
  int npoints     /* size of above array (1 + 3k) */
#endif
);

/**
 * Draw a smooth Catmull-Rom spline that passes through all the given points
 * (good for line/area charts). npoints must be >= 2 (two points draw a line).
 * The curve honors the GC line style and anti-aliasing.
 */
IError IDrawSpline (
#ifndef _NO_PROTO
  IImage image,   /* image */
  IGC gc,         /* graphics context */
  IPoint *points, /* points the curve passes through */
  int npoints     /* size of above array */
#endif
);

/**
 * Draws a rectangle onto the image of the specified width and
 * height using the graphic context's current line stlye (
 * see ISetLineStyle()).
 */
IError IDrawRectangle (
#ifndef _NO_PROTO
  IImage image,       /* image */
  IGC gc,             /* graphics context */
  int x,              /* x coordinate */
  int y,              /* y coordinate */
  unsigned int width, /* width */
  unsigned int height /* height */
#endif
);

/**
 * Draws a circle onto the image using the graphic context's
 * current line style (
 * see ISetLineStyle()).
 */
IError IDrawCircle (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* center x coordinate */
  int y,        /* center y coordinate */
  int r         /* angle radius */
#endif
);

/**
 * Draws an arc onto the image using the graphic context's
 * current line style (
 * see ISetLineStyle()).
 * In order to draw an arc that passes over 0 degrees, the first angle (a1)
 * should be negative and the second angle (a2) should be positive.
 */
IError IDrawArc (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* arc center x coordinate */
  int y,        /* arc center y coordinate */
  int r1,       /* angle 1 radius */
  int r2,       /* angle 2 radius */
  double a1,    /* first angle (in degrees) */
  double a2     /* second angle (in degrees) */
#endif
);

/**
 * Draws an arc and connects the arc to the center point
 * using the graphic context's
 * current line style (
 * see ISetLineStyle()).
 * This function is intended to be used drawing pie charts.
 * In order to draw an arc that passes over 0 degrees, the first angle (a1)
 * should be negative and the second angle (a2) should be positive.
 */
IError IDrawEnclosedArc (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* arc center x coordinate */
  int y,        /* arc center y coordinate */
  int r1,       /* angle 1 radius */
  int r2,       /* angle 2 radius */
  double a1,    /* first angle (in degrees) */
  double a2     /* second angle (in degrees) */
#endif
);

/**
 * Draws an ellipse onto the image using the graphic context's
 * current line style (
 * see ISetLineStyle()).
 */
IError IDrawEllipse (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* ellipse center x coordinate */
  int y,        /* ellipse center y coordinate */
  int r1,       /* x radius */
  int r2        /* y radius */
#endif
);

/**
 * Draws a circle onto the image using the graphic context's
 * current line style (
 * see ISetLineStyle()).
 */
IError IDrawCircle (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* circle center x coordinate */
  int y,        /* circle center y coordinate */
  int r         /* circle radius */
#endif
);


/**
 * Fills a rectangle on the image using the graphics context's
 * foreground color (see ISetForeground()).
 */
IError IFillRectangle (
#ifndef _NO_PROTO
  IImage image,       /* image */
  IGC gc,             /* graphics context */
  int x,              /* x coordinate */
  int y,              /* y coordinate */
  unsigned int width, /* width */
  unsigned int height /* height */
#endif
);

/**
 * Fill a polygon.  Only convex, non-intersecting polygons are supported.
 * Invalid polygons will return an error.
 */
IError IFillPolygon (
#ifndef _NO_PROTO
  IImage image,   /* image */
  IGC gc,         /* graphics context */
  IPoint *points, /* array of points */
  int npoints     /* size of above array */
#endif
);

/**
 * Fill an arc.
 * In order to draw an arc that passes over 0 degrees, the first angle (a1)
 * should be negative and the second angle (a2) should be positive.
 */
IError IFillArc (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* arc center x coordinate */
  int y,        /* arc center y coordinate */
  int r1,       /* angle 1 radius */
  int r2,       /* angle 2 radius */
  double a1,    /* first angle (in degrees) */
  double a2     /* second angle (in degrees) */
#endif
);

/**
 * Fill an ellipse.
 */
IError IFillEllipse (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* arc center x coordinate */
  int y,        /* arc center y coordinate */
  int r1,       /* angle 1 radius */
  int r2        /* angle 2 radius */
#endif
);

/**
 * Fill a circle.
 */
IError IFillCircle (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* arc center x coordinate */
  int y,        /* arc center y coordinate */
  int r         /* radius */
#endif
);

/**
 * Flood fill an area.
 */
IError IFloodFill (
#ifndef _NO_PROTO
  IImage image, /* image */
  IGC gc,       /* graphics context */
  int x,        /* x coordinate starting point */
  int y         /* y coordinate starting point */
#endif
);


#endif /* _ilib_h */
