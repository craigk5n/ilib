"""The :class:`Image` wrapper -- the heart of the binding."""

from __future__ import annotations

import os

from ._ffi import ffi, lib, libc
from .constants import (
    EXTENSION_FORMATS,
    Format,
    Option,
    TextDirection,
)
from .errors import IError, IlibError, check

__all__ = ["Image"]


def _points(points):
    """Build a C ``IPoint[]`` from an iterable of (x, y) pairs.

    Returns ``(cdata, count)``. Raises :class:`ValueError` for an empty list.
    """
    pts = [(int(x), int(y)) for x, y in points]
    if not pts:
        raise ValueError("at least one point is required")
    arr = ffi.new("IPoint[]", pts)
    return arr, len(pts)


def _text_bytes(text):
    data = text.encode("utf-8") if isinstance(text, str) else bytes(text)
    return data, len(data)


def _format_for(path, fmt):
    if fmt is not None:
        return Format(fmt)
    ext = os.path.splitext(str(path))[1].lstrip(".").lower()
    try:
        return EXTENSION_FORMATS[ext]
    except KeyError:
        raise IlibError(
            IError.InvalidFormat,
            f"cannot infer image format from {path!r}; pass format=...",
        )


class Image:
    """A raster image.

    Create a blank image with ``Image(width, height)``, load one with
    :meth:`open`, and release it with :meth:`free` or by using it as a context
    manager. Drawing methods take a :class:`~ilib.gc.GC`.
    """

    def __init__(self, width, height, options=Option.NONE, _handle=None):
        if _handle is not None:
            self._handle = _handle
            return
        handle = lib.ICreateImage(int(width), int(height), int(options))
        if not handle:
            raise MemoryError("ICreateImage() failed")
        self._handle = handle

    # -- construction ------------------------------------------------------
    @classmethod
    def open(cls, path, fmt=None, options=Option.NONE):
        """Read an image from ``path``.

        The format is inferred from the file extension unless ``fmt`` is given.
        """
        format_ = _format_for(path, fmt)
        fp = libc.fopen(str(path).encode("utf-8"), b"rb")
        if not fp:
            raise IlibError(IError.NoSuchFile, f"cannot open {path!r} for reading")
        try:
            out = ffi.new("IImage *")
            check(lib.IReadImageFile(fp, int(format_), int(options), out))
        finally:
            libc.fclose(fp)
        return cls(0, 0, _handle=out[0])

    # -- lifetime ----------------------------------------------------------
    def free(self):
        """Release the underlying image (idempotent)."""
        if getattr(self, "_handle", None):
            lib._IFreeImage(self._handle)
            self._handle = None

    def __del__(self):
        try:
            self.free()
        except Exception:  # pragma: no cover - destructor must not raise
            pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.free()
        return False

    @property
    def _as_parameter_(self):
        if not self._handle:
            raise ValueError("operation on a freed Image")
        return self._handle

    # -- geometry ----------------------------------------------------------
    @property
    def width(self):
        """Image width in pixels."""
        return int(lib.IImageWidth(self._as_parameter_))

    @property
    def height(self):
        """Image height in pixels."""
        return int(lib.IImageHeight(self._as_parameter_))

    @property
    def size(self):
        """``(width, height)`` in pixels."""
        return self.width, self.height

    # -- pixels ------------------------------------------------------------
    def set_pixel(self, x, y, red, green, blue):
        """Set the RGB color of a single pixel."""
        check(lib.ISetPixel(self._as_parameter_, int(x), int(y), int(red), int(green), int(blue)))
        return self

    def get_pixel(self, x, y):
        """Return the ``(red, green, blue)`` of a single pixel."""
        r = ffi.new("unsigned int *")
        g = ffi.new("unsigned int *")
        b = ffi.new("unsigned int *")
        check(lib.IGetPixel(self._as_parameter_, int(x), int(y), r, g, b))
        return int(r[0]), int(g[0]), int(b[0])

    def set_pixel_alpha(self, x, y, red, green, blue, alpha):
        """Set the RGBA color of a single pixel (alpha ignored on RGB images)."""
        check(
            lib.ISetPixelAlpha(
                self._as_parameter_, int(x), int(y),
                int(red), int(green), int(blue), int(alpha),
            )
        )
        return self

    def get_pixel_alpha(self, x, y):
        """Return the ``(red, green, blue, alpha)`` of a single pixel."""
        r = ffi.new("unsigned int *")
        g = ffi.new("unsigned int *")
        b = ffi.new("unsigned int *")
        a = ffi.new("unsigned int *")
        check(lib.IGetPixelAlpha(self._as_parameter_, int(x), int(y), r, g, b, a))
        return int(r[0]), int(g[0]), int(b[0]), int(a[0])

    # -- whole-image operations -------------------------------------------
    def duplicate(self):
        """Return a deep copy of this image."""
        out = ffi.new("IImage *")
        check(lib.IDuplicateImage(self._as_parameter_, out))
        return type(self)(0, 0, _handle=out[0])

    def copy_to(self, dest, gc, src_x=0, src_y=0, width=None, height=None,
                dest_x=0, dest_y=0):
        """Copy a region of this image onto ``dest`` at ``(dest_x, dest_y)``."""
        if width is None:
            width = self.width
        if height is None:
            height = self.height
        check(
            lib.ICopyImage(
                self._as_parameter_, dest._as_parameter_, gc._as_parameter_,
                int(src_x), int(src_y), int(width), int(height),
                int(dest_x), int(dest_y),
            )
        )
        return dest

    def copy_scaled_to(self, dest, gc, src_x, src_y, src_width, src_height,
                       dest_x, dest_y, dest_width, dest_height):
        """Copy a region of this image onto ``dest``, scaling to fit."""
        check(
            lib.ICopyImageScaled(
                self._as_parameter_, dest._as_parameter_, gc._as_parameter_,
                int(src_x), int(src_y), int(src_width), int(src_height),
                int(dest_x), int(dest_y), int(dest_width), int(dest_height),
            )
        )
        return dest

    def reduce_colors(self, max_colors=256):
        """Reduce the image to at most ``max_colors`` colors (median-cut)."""
        check(lib.IReduceColors(self._as_parameter_, int(max_colors)))
        return self

    # -- filters (whole-image point operations) ---------------------------
    def greyscale(self):
        """Desaturate to greyscale (Rec.601 luma), in place."""
        check(lib.IGreyscale(self._as_parameter_))
        return self

    grayscale = greyscale  # American-spelling alias

    def negate(self):
        """Invert the colors (each channel becomes 255 - value), in place."""
        check(lib.INegate(self._as_parameter_))
        return self

    def brightness_contrast(self, brightness=0, contrast=0):
        """Adjust brightness and contrast (each -100..100), in place."""
        check(lib.IBrightnessContrast(self._as_parameter_, int(brightness), int(contrast)))
        return self

    def gamma(self, gamma):
        """Apply gamma correction (``gamma`` > 0), in place."""
        check(lib.IGamma(self._as_parameter_, float(gamma)))
        return self

    def threshold(self, threshold):
        """Threshold each channel to black/white at ``threshold`` (0..255)."""
        check(lib.IThreshold(self._as_parameter_, int(threshold)))
        return self

    # -- transforms (geometric whole-image operations) --------------------
    def flip(self):
        """Flip vertically (top to bottom), in place."""
        check(lib.IFlip(self._as_parameter_))
        return self

    def flop(self):
        """Flop horizontally (left to right), in place."""
        check(lib.IFlop(self._as_parameter_))
        return self

    def rotate(self, degrees):
        """Rotate clockwise by a multiple of 90 degrees, in place.

        90/270 swap width and height; other multiples of 90 are accepted
        (negatives and 360 are normalized). A non-multiple raises IlibError.
        """
        check(lib.IRotate(self._as_parameter_, int(degrees)))
        return self

    def crop(self, x, y, width, height):
        """Crop to the rectangle ``(x, y, width, height)``, in place."""
        check(lib.ICrop(self._as_parameter_, int(x), int(y), int(width), int(height)))
        return self

    # -- convolution (area filters) ---------------------------------------
    def convolve(self, kernel, divisor=0.0, bias=0.0):
        """Apply a square convolution ``kernel``, in place.

        ``kernel`` is a square 2-D sequence of numbers (e.g. a list of equal-
        length rows) with an odd side length. ``divisor`` normalizes the result
        (0 = use the sum of the weights); ``bias`` is added afterward.
        """
        rows = [list(r) for r in kernel]
        size = len(rows)
        if size == 0 or any(len(r) != size for r in rows):
            raise ValueError("kernel must be a non-empty square 2-D sequence")
        flat = [float(v) for r in rows for v in r]
        cdata = ffi.new("double[]", flat)
        check(lib.IConvolve(self._as_parameter_, cdata, size, float(divisor), float(bias)))
        return self

    def blur(self, radius=1):
        """Box blur with the given ``radius`` (0 = no-op), in place."""
        check(lib.IBlur(self._as_parameter_, int(radius)))
        return self

    def gaussian_blur(self, sigma):
        """Gaussian blur with standard deviation ``sigma`` (> 0), in place."""
        check(lib.IGaussianBlur(self._as_parameter_, float(sigma)))
        return self

    def sharpen(self):
        """Sharpen (3x3 kernel), in place."""
        check(lib.ISharpen(self._as_parameter_))
        return self

    def edge_detect(self):
        """Detect edges (3x3 Laplacian), in place."""
        check(lib.IEdgeDetect(self._as_parameter_))
        return self

    def emboss(self):
        """Emboss (3x3 kernel), in place."""
        check(lib.IEmboss(self._as_parameter_))
        return self

    # -- transparency / comment -------------------------------------------
    def set_transparent(self, color):
        """Set the transparent color (relevant when writing GIF/XPM)."""
        check(lib.ISetTransparent(self._as_parameter_, int(color)))
        return self

    @property
    def transparent(self):
        """The transparent color handle, or ``None`` if unset."""
        out = ffi.new("IColor *")
        err = lib.IGetTransparent(self._as_parameter_, out)
        if err == IError.NoTransparentColor:
            return None
        check(err)
        return int(out[0])

    @property
    def comment(self):
        """The image comment string, or ``None`` if unset."""
        out = ffi.new("char **")
        err = lib.IGetComment(self._as_parameter_, out)
        check(err)
        if out[0] == ffi.NULL:
            return None
        return ffi.string(out[0]).decode("utf-8", "replace")

    @comment.setter
    def comment(self, text):
        check(lib.ISetComment(self._as_parameter_, text.encode("utf-8")))

    # -- output ------------------------------------------------------------
    def save(self, path, fmt=None, options=Option.NONE):
        """Write the image to ``path`` (format inferred from extension)."""
        format_ = _format_for(path, fmt)
        fp = libc.fopen(str(path).encode("utf-8"), b"wb")
        if not fp:
            raise IlibError(IError.ErrorWriting, f"cannot open {path!r} for writing")
        try:
            check(lib.IWriteImageFile(fp, self._as_parameter_, int(format_), int(options)))
        finally:
            libc.fclose(fp)
        return self

    # -- drawing: points / lines ------------------------------------------
    def draw_point(self, gc, x, y):
        """Draw a single point."""
        check(lib.IDrawPoint(self._as_parameter_, gc._as_parameter_, int(x), int(y)))
        return self

    def draw_line(self, gc, x1, y1, x2, y2):
        """Draw a line from ``(x1, y1)`` to ``(x2, y2)``."""
        check(lib.IDrawLine(self._as_parameter_, gc._as_parameter_, int(x1), int(y1), int(x2), int(y2)))
        return self

    def draw_polygon(self, gc, points):
        """Draw a polygon outline through ``points`` (an iterable of (x, y))."""
        arr, n = _points(points)
        check(lib.IDrawPolygon(self._as_parameter_, gc._as_parameter_, arr, n))
        return self

    def draw_bezier(self, gc, points):
        """Draw a chain of cubic Beziers; ``len(points)`` must be ``1 + 3k``."""
        arr, n = _points(points)
        check(lib.IDrawBezier(self._as_parameter_, gc._as_parameter_, arr, n))
        return self

    def draw_spline(self, gc, points):
        """Draw a Catmull-Rom spline through ``points`` (>= 2 points)."""
        arr, n = _points(points)
        check(lib.IDrawSpline(self._as_parameter_, gc._as_parameter_, arr, n))
        return self

    # -- drawing: shapes ---------------------------------------------------
    def draw_rectangle(self, gc, x, y, width, height):
        """Draw a rectangle outline."""
        check(lib.IDrawRectangle(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(width), int(height)))
        return self

    def draw_circle(self, gc, x, y, r):
        """Draw a circle outline centered at ``(x, y)`` with radius ``r``."""
        check(lib.IDrawCircle(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r)))
        return self

    def draw_ellipse(self, gc, x, y, r1, r2):
        """Draw an ellipse outline with x-radius ``r1`` and y-radius ``r2``."""
        check(lib.IDrawEllipse(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r1), int(r2)))
        return self

    def draw_arc(self, gc, x, y, r1, r2, a1, a2):
        """Draw an arc between angles ``a1`` and ``a2`` (degrees)."""
        check(lib.IDrawArc(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r1), int(r2), float(a1), float(a2)))
        return self

    def draw_enclosed_arc(self, gc, x, y, r1, r2, a1, a2):
        """Draw an arc connected to its center (pie-chart wedge outline)."""
        check(lib.IDrawEnclosedArc(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r1), int(r2), float(a1), float(a2)))
        return self

    # -- drawing: fills ----------------------------------------------------
    def fill_rectangle(self, gc, x, y, width, height):
        """Fill a rectangle."""
        check(lib.IFillRectangle(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(width), int(height)))
        return self

    def fill_polygon(self, gc, points):
        """Fill a polygon through ``points``."""
        arr, n = _points(points)
        check(lib.IFillPolygon(self._as_parameter_, gc._as_parameter_, arr, n))
        return self

    def fill_circle(self, gc, x, y, r):
        """Fill a circle."""
        check(lib.IFillCircle(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r)))
        return self

    def fill_ellipse(self, gc, x, y, r1, r2):
        """Fill an ellipse."""
        check(lib.IFillEllipse(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r1), int(r2)))
        return self

    def fill_arc(self, gc, x, y, r1, r2, a1, a2):
        """Fill a pie wedge between angles ``a1`` and ``a2`` (degrees)."""
        check(lib.IFillArc(self._as_parameter_, gc._as_parameter_, int(x), int(y), int(r1), int(r2), float(a1), float(a2)))
        return self

    def flood_fill(self, gc, x, y):
        """Flood fill starting at ``(x, y)``."""
        check(lib.IFloodFill(self._as_parameter_, gc._as_parameter_, int(x), int(y)))
        return self

    # -- drawing: text -----------------------------------------------------
    def draw_string(self, gc, x, y, text):
        """Draw ``text`` with its lower-left corner at ``(x, y)``."""
        data, length = _text_bytes(text)
        check(lib.IDrawString(self._as_parameter_, gc._as_parameter_, int(x), int(y), data, length))
        return self

    def draw_string_rotated(self, gc, x, y, text, direction=TextDirection.LEFT_TO_RIGHT):
        """Draw ``text`` in one of the cardinal :class:`TextDirection` directions."""
        data, length = _text_bytes(text)
        check(
            lib.IDrawStringRotated(
                self._as_parameter_, gc._as_parameter_, int(x), int(y),
                data, length, int(TextDirection(direction)),
            )
        )
        return self

    def draw_string_angle(self, gc, x, y, text, angle):
        """Draw ``text`` rotated by ``angle`` degrees (counterclockwise)."""
        data, length = _text_bytes(text)
        check(
            lib.IDrawStringRotatedAngle(
                self._as_parameter_, gc._as_parameter_, int(x), int(y),
                data, length, float(angle),
            )
        )
        return self
