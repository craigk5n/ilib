"""Low-level cffi bindings for the Ilib C library (ABI / dlopen mode).

This module loads the installed ``libilib`` shared library at runtime and
declares the C ABI with :mod:`cffi`. It deliberately uses ABI mode (no C
compiler required at install time): only an installed ``libilib`` is needed.

Most users should import the high-level wrappers from :mod:`ilib` instead of
using ``ffi``/``lib`` directly.
"""

from __future__ import annotations

import ctypes.util
import os

from cffi import FFI

# --- C declarations ---------------------------------------------------------
# Enums are declared as plain ``int`` (their ABI representation); the symbolic
# values live in ilib.constants / ilib.errors as Python IntEnums. ``FILE`` is
# left opaque -- we only ever pass pointers obtained from fopen() to the
# read/write functions and never dereference them.
_CDEF = """
typedef void *IImage;
typedef void *IAnimation;
typedef void *IFont;
typedef void *IGC;
typedef unsigned int IColor;
typedef unsigned int IOptions;
typedef int IError;
typedef int IFileFormat;
typedef int ILineStyle;
typedef int IFillStyle;
typedef int ITextStyle;
typedef int ITextDirection;
typedef int IBlendMode;
typedef struct { int x; int y; } IPoint;

typedef struct _ilib_FILE FILE;
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);

char *IErrorString(IError err);

IImage ICreateImage(unsigned int width, unsigned int height,
                    unsigned int options);
IError IDuplicateImage(IImage image, IImage *image_return);
IError ICopyImage(IImage source, IImage dest, IGC gc, int src_x, int src_y,
                  unsigned int width, unsigned int height,
                  int dest_x, int dest_y);
IError ICopyImageScaled(IImage source, IImage dest, IGC gc, int src_x,
                        int src_y, unsigned int src_width,
                        unsigned int src_height, int dest_x, int dest_y,
                        unsigned int dest_width, unsigned int dest_height);
IError IReduceColors(IImage image, unsigned int max_colors);
IError IDither(IImage image, unsigned int max_colors);

IError IGreyscale(IImage image);
IError INegate(IImage image);
IError IBrightnessContrast(IImage image, int brightness, int contrast);
IError IGamma(IImage image, double gamma);
IError IThreshold(IImage image, unsigned int threshold);
IError INormalize(IImage image);
IError ISepia(IImage image);
IError IOpacity(IImage image, double factor);

IError IFlip(IImage image);
IError IFlop(IImage image);
IError IRotate(IImage image, int degrees);
IError ICrop(IImage image, int x, int y, unsigned int width,
             unsigned int height);

IError IConvolve(IImage image, const double *kernel, unsigned int size,
                 double divisor, double bias);
IError IBlur(IImage image, unsigned int radius);
IError IGaussianBlur(IImage image, double sigma);
IError ISharpen(IImage image);
IError IEdgeDetect(IImage image);
IError IEmboss(IImage image);

IError IResize(IImage image, unsigned int width, unsigned int height);
IError IResizeFiltered(IImage image, unsigned int width, unsigned int height,
                       int filter);
IError IRotateAngle(IImage image, double degrees, IColor background);

IError ITrim(IImage image, unsigned int tolerance);
IError IBorder(IImage image, unsigned int width, IColor color);
IImage IAppend(IImage *images, int count, int horizontal, IColor background);
IImage IMontage(IImage *images, int count, int columns, int spacing,
                IColor background);

IError _IFreeImage(IImage image);
unsigned int IImageHeight(IImage image);
unsigned int IImageWidth(IImage image);

IError ISetPixel(IImage image, int x, int y, unsigned int red,
                 unsigned int green, unsigned int blue);
IError IGetPixel(IImage image, int x, int y, unsigned int *red_return,
                 unsigned int *green_return, unsigned int *blue_return);
IError ISetPixelAlpha(IImage image, int x, int y, unsigned int red,
                      unsigned int green, unsigned int blue,
                      unsigned int alpha);
IError IGetPixelAlpha(IImage image, int x, int y, unsigned int *red_return,
                      unsigned int *green_return, unsigned int *blue_return,
                      unsigned int *alpha_return);

IError ISetComment(IImage image, char *comment);
IError IGetComment(IImage image, char **comment);

IError IFileType(char *file, IFileFormat *format_return);
IError IReadImageFile(FILE *fp, IFileFormat format, IOptions options,
                      IImage *image_return);
IError IWriteImageFile(FILE *fp, IImage image, IFileFormat format,
                       IOptions options);

IAnimation ICreateAnimation(void);
IError _IFreeAnimation(IAnimation anim);
IError IAddAnimationFrame(IAnimation anim, IImage frame, int delay_ms);
int IAnimationFrameCount(IAnimation anim);
IImage IAnimationFrame(IAnimation anim, int index);
int IAnimationFrameDelay(IAnimation anim, int index);
IError ISetAnimationLoopCount(IAnimation anim, int loops);
int IAnimationLoopCount(IAnimation anim);
IAnimation IReadAnimationFile(FILE *fp, IFileFormat format);
IError IWriteAnimationFile(FILE *fp, IAnimation anim, IFileFormat format,
                           IOptions options);

IGC ICreateGC(void);
IError _IFreeGC(IGC gc);

IError ILoadFontFromFile(char *name, char *path, IFont *font_return);
IError ILoadFontFromFileTTF(char *name, char *path, unsigned int pixel_size,
                            IFont *font_return);
IError _IFreeFont(IFont font);
IError IFontSize(IFont font, unsigned int *height_return);

IError ITextWidth(IGC gc, IFont font, char *text, unsigned int len,
                  unsigned int *width_return);
IError ITextHeight(IGC gc, IFont font, char *text, unsigned int len,
                   unsigned int *height_return);
IError ITextDimensions(IGC gc, IFont font, char *text, unsigned int len,
                       unsigned int *width_return,
                       unsigned int *height_return);
IError ICalculateTextCoordinates(IGC gc, IFont font, char *text,
                                 unsigned int len, int anchor_x, int anchor_y,
                                 int halign, int valign, int *x_return,
                                 int *y_return);

IError IArcProperties(IGC gc, int x, int y, int r1, int r2, double a1,
                      double a2, int *a1_x, int *a1_y, int *a2_x, int *a2_y,
                      int *middle_x, int *middle_y);

IColor IAllocColor(unsigned int red, unsigned int green, unsigned int blue);
IColor IAllocColorAlpha(unsigned int red, unsigned int green,
                        unsigned int blue, unsigned int alpha);
IError IAllocNamedColor(char *colorname, IColor *color_ret);
IError _IFreeColor(IColor color);

IError ISetTransparent(IImage image, IColor color);
IError IGetTransparent(IImage image, IColor *color);

IError ISetForeground(IGC gc, IColor color);
IError ISetBlendMode(IGC gc, IBlendMode mode);
IError ISetAntiAlias(IGC gc, int on);
IError ISetBackground(IGC gc, IColor color);
IError ISetFont(IGC gc, IFont font);
IError ISetLineWidth(IGC gc, unsigned int line_width);
IError ISetLineStyle(IGC gc, ILineStyle line_style);
IError ISetTextStyle(IGC gc, ITextStyle text_style);

IError IDrawString(IImage image, IGC gc, int x, int y, char *text,
                   unsigned int len);
IError IDrawStringRotated(IImage image, IGC gc, int x, int y, char *text,
                          unsigned int len, ITextDirection direction);
IError IDrawStringRotatedAngle(IImage image, IGC gc, int x, int y, char *text,
                               unsigned int len, double angle);

IError IDrawPoint(IImage image, IGC gc, int x, int y);
IError IDrawLine(IImage image, IGC gc, int x1, int y1, int x2, int y2);
IError IDrawPolygon(IImage image, IGC gc, IPoint *points, int npoints);
IError IDrawBezier(IImage image, IGC gc, IPoint *points, int npoints);
IError IDrawSpline(IImage image, IGC gc, IPoint *points, int npoints);
IError IDrawRectangle(IImage image, IGC gc, int x, int y, unsigned int width,
                      unsigned int height);
IError IDrawCircle(IImage image, IGC gc, int x, int y, int r);
IError IDrawArc(IImage image, IGC gc, int x, int y, int r1, int r2, double a1,
                double a2);
IError IDrawEnclosedArc(IImage image, IGC gc, int x, int y, int r1, int r2,
                        double a1, double a2);
IError IDrawEllipse(IImage image, IGC gc, int x, int y, int r1, int r2);

IError IFillRectangle(IImage image, IGC gc, int x, int y, unsigned int width,
                      unsigned int height);
IError IFillPolygon(IImage image, IGC gc, IPoint *points, int npoints);
IError IFillArc(IImage image, IGC gc, int x, int y, int r1, int r2, double a1,
                double a2);
IError IFillEllipse(IImage image, IGC gc, int x, int y, int r1, int r2);
IError IFillCircle(IImage image, IGC gc, int x, int y, int r);
IError IFloodFill(IImage image, IGC gc, int x, int y);

typedef void *IChart;
IChart ICreateChart(int type, unsigned int width, unsigned int height);
IError _IFreeChart(IChart chart);
IError IChartSetTitle(IChart chart, const char *title);
IError IChartSetAxisLabels(IChart chart, const char *x_label,
                           const char *y_label);
IError IChartSetFont(IChart chart, IFont font);
IError IChartSetBackground(IChart chart, IColor color);
IError IChartSetCategories(IChart chart, const char **labels, int count);
IError IChartSetRange(IChart chart, double ymin, double ymax);
IError IChartSetStacked(IChart chart, int stacked);
IError IChartSetLogScale(IChart chart, int on);
IError IChartSetValueLabels(IChart chart, int on);
IError IChartSetMarkers(IChart chart, int on);
IError IChartSetGrid(IChart chart, int on);
IError IChartSetLegend(IChart chart, int on);
IError IChartAddSeries(IChart chart, const char *label, const double *values,
                       int count, IColor color);
IError IChartAddXYSeries(IChart chart, const char *label,
                         const double *xvalues, const double *yvalues,
                         int count, IColor color);
IImage IChartRender(IChart chart);
"""

# Candidate shared-library names tried (in order) when ILIB_LIBRARY is unset
# and ctypes.util.find_library() comes up empty.
_CANDIDATES = (
    "libilib.so",
    "libilib.so.1",
    "libilib.dylib",
    "libilib.1.dylib",
    "ilib.dll",
    "libilib.dll",
)


def _find_library():
    """Return the shared-library name/path to dlopen.

    Resolution order:

    1. ``$ILIB_LIBRARY`` -- an explicit path or soname (useful for tests and
       for pointing at an uninstalled build tree).
    2. :func:`ctypes.util.find_library` (``"ilib"``).
    3. A small list of conventional platform filenames.
    """
    env = os.environ.get("ILIB_LIBRARY")
    if env:
        return env
    found = ctypes.util.find_library("ilib")
    if found:
        return found
    return None


def _load():
    ffi = FFI()
    ffi.cdef(_CDEF)
    libc = ffi.dlopen(None)  # fopen/fclose from the process's symbol table

    name = _find_library()
    if name is not None:
        lib = ffi.dlopen(name)
        return ffi, lib, libc

    # No explicit/located library: try the conventional names directly.
    errors = []
    for candidate in _CANDIDATES:
        try:
            lib = ffi.dlopen(candidate)
            return ffi, lib, libc
        except OSError as exc:  # pragma: no cover - platform dependent
            errors.append(f"{candidate}: {exc}")

    raise OSError(
        "Could not locate the Ilib shared library. Install Ilib, or set the "
        "ILIB_LIBRARY environment variable to its path "
        "(e.g. /usr/local/lib/libilib.so or build/src/libilib.so).\n"
        "Tried:\n  " + "\n  ".join(errors)
    )


ffi, lib, libc = _load()

NULL = ffi.NULL
