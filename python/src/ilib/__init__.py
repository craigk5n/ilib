"""Python bindings for Ilib, a C raster-image library.

These bindings load an installed ``libilib`` shared library at runtime via
:mod:`cffi` (ABI mode -- no C compiler needed to install this package, only the
Ilib C library). If the library cannot be found, set the ``ILIB_LIBRARY``
environment variable to its path.

Quick start::

    import ilib

    img = ilib.Image(200, 100)
    with ilib.GC() as gc:
        gc.set_antialias(True)
        gc.set_foreground(ilib.named_color("steelblue"))
        img.fill_rectangle(gc, 0, 0, 200, 100)
        gc.set_foreground(ilib.named_color("white"))
        img.draw_circle(gc, 100, 50, 40)
    img.save("out.png")
"""

from __future__ import annotations

from .color import (
    BLACK,
    WHITE,
    alloc_color,
    alloc_color_alpha,
    free_color,
    named_color,
)
from .constants import (
    BlendMode,
    FillStyle,
    Format,
    LineStyle,
    Option,
    TextDirection,
    TextStyle,
)
from .errors import IError, IlibError, error_string
from .font import Font
from .gc import GC, ArcPoints
from .image import Image

__version__ = "0.1.0"

__all__ = [
    "Image",
    "GC",
    "Font",
    "ArcPoints",
    # colors
    "alloc_color",
    "alloc_color_alpha",
    "named_color",
    "free_color",
    "BLACK",
    "WHITE",
    # enums
    "Format",
    "Option",
    "LineStyle",
    "FillStyle",
    "TextStyle",
    "TextDirection",
    "BlendMode",
    # errors
    "IError",
    "IlibError",
    "error_string",
    "__version__",
]
