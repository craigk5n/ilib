"""Color allocation helpers.

An Ilib ``IColor`` is a small integer index into a library-global color table,
not a pointer, so a color is represented in Python as a plain :class:`int`.
Colors are cheap; they are not freed automatically (mirroring typical Ilib
usage). Use :func:`free_color` if you allocate a great many short-lived colors.
"""

from __future__ import annotations

from ._ffi import ffi, lib
from .constants import BLACK_PIXEL, WHITE_PIXEL
from .errors import IError, IlibError, check

__all__ = [
    "BLACK",
    "WHITE",
    "alloc_color",
    "alloc_color_alpha",
    "named_color",
    "free_color",
]

BLACK = BLACK_PIXEL
WHITE = WHITE_PIXEL


def _check_channel(value, name):
    if not 0 <= int(value) <= 255:
        raise ValueError(f"{name} must be in 0..255, got {value!r}")
    return int(value)


def alloc_color(red, green, blue):
    """Allocate an opaque RGB color and return its handle (an ``int``)."""
    r = _check_channel(red, "red")
    g = _check_channel(green, "green")
    b = _check_channel(blue, "blue")
    return int(lib.IAllocColor(r, g, b))


def alloc_color_alpha(red, green, blue, alpha):
    """Allocate a translucent RGBA color (alpha 0=transparent..255=opaque)."""
    r = _check_channel(red, "red")
    g = _check_channel(green, "green")
    b = _check_channel(blue, "blue")
    a = _check_channel(alpha, "alpha")
    return int(lib.IAllocColorAlpha(r, g, b, a))


def named_color(name):
    """Allocate a color by name (e.g. ``"blue"``) and return its handle.

    Raises :class:`~ilib.errors.IlibError` (``InvalidColorName``) if the name
    is not recognized.
    """
    out = ffi.new("IColor *")
    check(lib.IAllocNamedColor(name.encode("utf-8"), out))
    return int(out[0])


def free_color(color):
    """Free a previously allocated color handle."""
    err = lib._IFreeColor(int(color))
    if err != IError.NoError:
        raise IlibError(err)
