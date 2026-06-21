"""Enumerations mirroring the Ilib constants in ``Ilib.h``."""

from __future__ import annotations

from enum import IntEnum, IntFlag


class Format(IntEnum):
    """Image file formats (``IFORMAT_*``)."""

    GIF = 0
    PPM = 1
    PGM = 2
    PBM = 3
    XPM = 4
    XBM = 5
    PNG = 6
    JPEG = 7
    BMP = 8
    WEBP = 9
    AVIF = 10


# Map common filename extensions to a Format, used by Image.open()/save() when
# a format is not given explicitly. (Mirrors the C IFileType() extension map.)
EXTENSION_FORMATS = {
    "gif": Format.GIF,
    "ppm": Format.PPM,
    "pgm": Format.PGM,
    "pbm": Format.PBM,
    "xpm": Format.XPM,
    "xbm": Format.XBM,
    "png": Format.PNG,
    "jpg": Format.JPEG,
    "jpeg": Format.JPEG,
    "bmp": Format.BMP,
    "webp": Format.WEBP,
    "avif": Format.AVIF,
}


class Option(IntFlag):
    """Option flags for :class:`~ilib.image.Image` creation and writing."""

    NONE = 0x0000
    # ICreateImage()
    GREYSCALE = 0x0001
    GRAYSCALE = 0x0001
    ALPHA = 0x0004
    # IWriteImageFile()
    ASCII = 0x0001
    INTERLACED = 0x0002


class LineStyle(IntEnum):
    """Line drawing styles (``ILINE_*``)."""

    SOLID = 0
    ON_OFF_DASH = 1
    DOUBLE_DASH = 2


class FillStyle(IntEnum):
    """Fill styles (``IFILL_*``)."""

    SOLID = 0
    TILED = 1
    STIPPLED = 2
    OPAQUE_STIPPLED = 3


class TextStyle(IntEnum):
    """Text drawing styles (``ITEXT_*``)."""

    NORMAL = 0
    ETCHED_IN = 1
    ETCHED_OUT = 2
    SHADOWED = 3


class TextDirection(IntEnum):
    """Text drawing directions (``ITEXT_LEFT_TO_RIGHT`` etc.)."""

    LEFT_TO_RIGHT = 0
    BOTTOM_TO_TOP = 1
    TOP_TO_BOTTOM = 2


class BlendMode(IntEnum):
    """Pixel compositing modes (``IBLEND_*``)."""

    REPLACE = 0
    OVER = 1


class ResizeFilter(IntEnum):
    """Resampling filters for :meth:`ilib.image.Image.resize` (``IRESIZE_*``)."""

    NEAREST = 0
    BILINEAR = 1
    BICUBIC = 2
    AREA = 3
    AUTO = 4


class ChartType(IntEnum):
    """Chart types (``ICHART_*``)."""

    LINE = 0
    BAR = 1
    PIE = 2
    SCATTER = 3
    AREA = 4


# Predefined color handles (indexes into the library's color table).
BLACK_PIXEL = 0
WHITE_PIXEL = 1
