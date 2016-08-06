#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <Ilib.h>

static int
not_here(char *s)
{
    croak("%s not implemented on this architecture", s);
    return -1;
}

static char *
str_constant(char *name)
{

  switch ( *name ) {
    case 'I':
      if (strEQ(name, "ILIB_URL"))
        return ILIB_URL;
      if (strEQ(name, "ILIB_VERSION"))
        return ILIB_VERSION;
      if (strEQ(name, "ILIB_VERSION_DATE"))
        return ILIB_VERSION_DATE;
    break;
  }

  return "";
}

static double
constant(char *name, int arg)
{
    errno = 0;
    switch (*name) {
    case 'A':
	break;
    case 'B':
	break;
    case 'C':
	break;
    case 'D':
	break;
    case 'E':
	break;
    case 'F':
	break;
    case 'G':
	break;
    case 'H':
	break;
    case 'I':
	if (strEQ(name, "ILIB_URL"))
	    return atof(ILIB_URL);
	if (strEQ(name, "ILIB_VERSION"))
	    return atof(ILIB_VERSION);
	if (strEQ(name, "ILIB_VERSION_DATE"))
	    return atof(ILIB_VERSION_DATE);
	if (strEQ(name, "ILIB_MAJOR_VERSION"))
	    return ILIB_MAJOR_VERSION;
	if (strEQ(name, "ILIB_MICRO_VERSION"))
	    return ILIB_MICRO_VERSION;
	if (strEQ(name, "ILIB_MINOR_VERSION"))
	    return ILIB_MINOR_VERSION;

	if (strEQ(name, "IOPTION_ASCII"))
	    return IOPTION_ASCII;
	if (strEQ(name, "IOPTION_GRAYSCALE"))
	    return IOPTION_GRAYSCALE;
	if (strEQ(name, "IOPTION_GREYSCALE"))
	    return IOPTION_GREYSCALE;
	if (strEQ(name, "IOPTION_INTERLACED"))
	    return IOPTION_INTERLACED;
	if (strEQ(name, "IOPTION_NONE"))
	    return IOPTION_NONE;

	if (strEQ(name, "IBLACK_PIXEL"))
	    return IBLACK_PIXEL;
	if (strEQ(name, "IWHITE_PIXEL"))
	    return IWHITE_PIXEL;

	if (strEQ(name, "INUM_FORMATS"))
	    return INUM_FORMATS;
	if (strEQ(name, "IFORMAT_GIF"))
	    return IFORMAT_GIF;
	if (strEQ(name, "IFORMAT_PPM"))
	    return IFORMAT_PPM;
	if (strEQ(name, "IFORMAT_PGM"))
	    return IFORMAT_PGM;
	if (strEQ(name, "IFORMAT_PBM"))
	    return IFORMAT_PBM;
	if (strEQ(name, "IFORMAT_XPM"))
	    return IFORMAT_XPM;
	if (strEQ(name, "IFORMAT_XBM"))
	    return IFORMAT_XBM;
	if (strEQ(name, "IFORMAT_PNG"))
	    return IFORMAT_PNG;
	if (strEQ(name, "IFORMAT_JPEG"))
	    return IFORMAT_JPEG;

	if (strEQ(name, "ILINE_SOLID"))
	    return ILINE_SOLID;
	if (strEQ(name, "ILINE_ON_OFF_DASH"))
	    return ILINE_ON_OFF_DASH;
	if (strEQ(name, "ILINE_DOUBLE_DASH"))
	    return ILINE_DOUBLE_DASH;

	if (strEQ(name, "IFILL_SOLID"))
	    return IFILL_SOLID;
	if (strEQ(name, "IFILL_TILED"))
	    return IFILL_TILED;
	if (strEQ(name, "IFILL_STIPPLED"))
	    return IFILL_STIPPLED;
	if (strEQ(name, "IFILL_OPAQUE_STIPPLED"))
	    return IFILL_OPAQUE_STIPPLED;

	if (strEQ(name, "ITEXT_NORMAL"))
	    return ITEXT_NORMAL;
	if (strEQ(name, "ITEXT_ETCHED_IN"))
	    return ITEXT_ETCHED_IN;
	if (strEQ(name, "ITEXT_ETCHED_OUT"))
	    return ITEXT_ETCHED_OUT;
	if (strEQ(name, "ITEXT_SHADOWED"))
	    return ITEXT_SHADOWED;

	if (strEQ(name, "ITEXT_LEFT_TO_RIGHT"))
	    return ITEXT_LEFT_TO_RIGHT;
	if (strEQ(name, "ITEXT_BOTTOM_TO_TOP"))
	    return ITEXT_BOTTOM_TO_TOP;
	if (strEQ(name, "ITEXT_TOP_TO_BOTTOM"))
	    return ITEXT_TOP_TO_BOTTOM;

	break;

    case 'J':
	break;
    case 'K':
	break;
    case 'L':
	break;
    case 'M':
	break;
    case 'N':
	break;
    case 'O':
	break;
    case 'P':
	break;
    case 'Q':
	break;
    case 'R':
	break;
    case 'S':
	break;
    case 'T':
	break;
    case 'U':
	break;
    case 'V':
	break;
    case 'W':
	break;
    case 'X':
	break;
    case 'Y':
	break;
    case 'Z':
	break;
    }
    errno = EINVAL;
    return 0;

not_there:
    errno = ENOENT;
    return 0;
}

typedef IImage Ilib__IImage;
typedef IGC Ilib__IGC;
typedef IColor Ilib__IColor;
typedef IFont * Ilib__IFont;

MODULE = Ilib		PACKAGE = Ilib		


double
constant(name,arg)
	char *		name
	int		arg

char *
str_constant(name)
	char *		name


char *
IErrorString(err)
	int err
	CODE:
	RETVAL = IErrorString ( err );
	OUTPUT:
	RETVAL

Ilib::IImage
ICreateImage ( width=480, height=640, options=0 )
	unsigned int width
	unsigned int height
	unsigned int options
	CODE:
	RETVAL = ICreateImage ( width, height, options );
	OUTPUT:
	RETVAL

int
IWriteImageFile ( fp, image, format=IFORMAT_GIF, options=0 )
	FILE *fp
	Ilib::IImage image
	unsigned int format
	unsigned int options
	CODE:
	RETVAL = IWriteImageFile ( fp, image, format, options );
	OUTPUT:
	RETVAL

Ilib::IGC
ICreateGC ( )
	CODE:
	RETVAL = ICreateGC ();
	OUTPUT:
	RETVAL

Ilib::IColor
IAllocColor ( red, green, blue )
	unsigned int red
	unsigned int green
	unsigned int blue
	CODE:
	RETVAL = IAllocColor ( red, green, blue );
	OUTPUT:
	RETVAL

Ilib::IColor
IAllocNamedColor ( colorname )
	char *colorname
	CODE:
	{
		IColor ret;
		IAllocNamedColor ( colorname, &ret );
		RETVAL = ret;
	}
	OUTPUT:
	RETVAL

unsigned int
ISetBackground ( gc, color )
	Ilib::IGC gc
	Ilib::IColor color
	CODE:
	RETVAL = ISetBackground ( gc, color );
	OUTPUT:
	RETVAL

unsigned int
IFillRectangle ( image, gc, x, y, width, height )
	Ilib::IImage image
	Ilib::IGC gc
	int x
	int y
	unsigned int width
	unsigned int height
	CODE:
	RETVAL = IFillRectangle ( image, gc, x, y, width, height );
	OUTPUT:
	RETVAL

unsigned int
ISetForeground ( gc, color )
	Ilib::IGC gc
	Ilib::IColor color
	CODE:
	RETVAL = ISetForeground ( gc, color );
	OUTPUT:
	RETVAL

unsigned int
ILoadFontFromFile ( name, path, font_return )
	char *name
	char *path
	Ilib::IFont & font_return
	CODE:
	RETVAL = ILoadFontFromFile ( name, path, &font_return );
	OUTPUT:
	RETVAL

void
IImage_DESTORY ( image )
	Ilib::IImage image
	CODE:
	_IFreeImage ( image );

