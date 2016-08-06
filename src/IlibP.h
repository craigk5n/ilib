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
#define IMAGIC_IMAGE		467
#define IMAGIC_GC		333
#define IMAGIC_COLOR		847
#define IMAGIC_FONT		104

#ifndef PI
#define PI 3.14159265358979323846
#endif /* PI */


/*
** Default comment for images
*/
#define IDEFAULT_COMMENT	"Creator: Ilib http://ilib.sourceforge.net/"

/*
** Structures
*/
typedef struct {
  unsigned int magic;		/* memory verification */
  unsigned char red;		/* red value (0-255) */
  unsigned char green;		/* green value (0-255) */
  unsigned char blue;		/* blue value (0-255) */
  unsigned long value;		/* pixel value */
} IColorP;

typedef struct {
  unsigned int magic;		/* memory verification */
  int width;			/* width */
  int height;			/* height */
  char *comments;		/* comments */
  unsigned char *data;		/* image data */
  unsigned short interlaced;	/* interlaced output? */
  unsigned short greyscale;	/* greyscale? */
  IColorP *transparent;		/* transparent color */
} IImageP;

typedef struct {
  unsigned int magic;		/* memory verification */
  char *name;
} IFontP;

typedef struct {
  unsigned int magic;		/* memory verification */
  IColorP *foreground;		/* foreground color */
  IColorP *background;		/* background color */
  IFontP *font;			/* font */
  unsigned short antialiased;	/* use anti-aliasing (size is halfed) */
  unsigned int line_width;	/* line width */
  ILineStyle line_style;	/* line style */
  ITextStyle text_style;	/* line style */
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

typedef struct {
  char *name;
  IError (*read_func) (
#ifndef _NO_PROTO
    FILE *fp,			/* output file pointer */
    IOptions options,		/* read options */
    IImageP **image_return	/* returned image */
#endif
  );
  IError (*write_func) (
#ifndef _NO_PROTO
    FILE *fp,			/* output file pointer */
    IImageP *image,		/* image to save */
    IOptions options		/* write options */
#endif
  );
  int maxdepth;			/* max depth (1=b/w, 8=256 colors) */
  int grey;			/* is grey? */
} IFormatDef;
#ifdef IIncludeFileFormats
static IFormatDef IFileFormats[] = {
  { /* IFORMAT_GIF */ "GIF", _IReadGIF, _IWriteGIF, 8, 0 },
  { /* IFORMAT_PPM */ "PPM", _IReadPPM, _IWritePPM, 24, 0 },
  { /* IFORMAT_PGM */ "PGM", _IReadPPM, _IWritePGM, 8, 1 },
  { /* IFORMAT_PBM */ "PBM", NULL, NULL, 1, 1 },
  { /* IFORMAT_XPM */ "XPM", _IReadXPM, _IWriteXPM, 24, 0 },
  { /* IFORMAT_XBM */ "XBM", NULL, NULL, 1, 1 },
#ifdef HAVE_PNGLIB
  { /* IFORMAT_PNG */ "PNG", _IReadPNG, _IWritePNG, 24, 0 },
#else
  { /* IFORMAT_PNG */ "PNG", NULL, NULL, 24, 0 },
#endif
#ifdef HAVE_JPEGLIB
  { /* IFORMAT_JPEG */ "JPEG", _IReadJPEG, _IWriteJPEG, 24, 0 },
#else
  { /* IFORMAT_JPEG */ "JPEG", NULL, NULL, 24, 0 },
#endif
  { /* IFORMAT_BMP */ "BMP", _IReadBMP, NULL, 24, 0 },
};
#endif /* IIncludeFileFormats */

IColorP *_IGetColor (
#ifndef _NO_PROTO
  int color			/* int index to translate */
#endif
);

IError _IFontBDFGetSize (
#ifndef _NO_PROTO
  char *name,			/* font name */
  unsigned int *height_return	/* out: height of font */
#endif
);

/*
** Define a function to draw a point.  Must be inline because this
** is typically within multiple for loops and we don't want all those
** extra stack pushes and pops slowing us down...
** Arguments are ( ImageP *, IGCP *, int, int ).
*/
#define _IDrawPoint _ISetPoint
#define _ISetPoint(i,g,x,y) \
	{ \
	  unsigned char *ptrX;\
	  if ( (x) >= 0 && (x) < i->width && (y) >= 0 && (y) < i->height ) {\
            if ( i->greyscale ) {\
	      ptrX = i->data + ( (y) * i->width ) + (x); \
	      *ptrX = g->foreground->red; \
            } else {\
	      ptrX = i->data + ( (y) * i->width * 3 ) + ( (x) * 3 ); \
	      *ptrX = g->foreground->red; \
	      *(ptrX + 1) = g->foreground->green; \
	      *(ptrX + 2) = g->foreground->blue; \
            }\
	  }\
	}

#define _ISetPointRGB(i,x,y,r,g,b) \
	{ \
	  unsigned char *ptrX;\
	  if ( (x) >= 0 && (x) < i->width && (y) >= 0 && (y) < i->height ) {\
            if ( i->greyscale ) {\
	      ptrX = i->data + ( (y) * i->width ) + (x); \
	      *ptrX = r; \
            } else {\
	      ptrX = i->data + ( (y) * i->width * 3 ) + ( (x) * 3 ); \
	      *ptrX = r; \
	      *(ptrX + 1) = g; \
	      *(ptrX + 2) = b; \
            }\
	  }\
	}

#define _IGetPointColor(i,x,y,c) \
	{ \
	  unsigned char *ptrX;\
	  if ( (x) >= 0 && (x) < i->width && (y) >= 0 && (y) < i->height ) {\
            if ( i->greyscale ) {\
	      ptrX = i->data + ( (y) * i->width ) + (x); \
	      c.red = c.green = c.blue = *ptrX; \
            } else {\
	      ptrX = i->data + ( (y) * i->width * 3 ) + ( (x) * 3 ); \
	      c.red = *ptrX; \
	      c.green = *(ptrX + 1); \
	      c.blue = *(ptrX + 2); \
            }\
	  }\
	}

#define _IColorsMatch(c1,c2) \
	( c1.red == c2.red && c1.green == c2.green && c1.blue == c2.blue )

#endif /* _ilibp_h */

