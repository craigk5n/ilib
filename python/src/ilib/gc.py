"""The graphics context (:class:`GC`) wrapper."""

from __future__ import annotations

from collections import namedtuple

from ._ffi import ffi, lib
from .constants import BlendMode, LineStyle, TextStyle
from .errors import check

__all__ = ["GC", "ArcPoints"]

#: Result of :meth:`GC.arc_properties` -- the (x, y) of the two angle endpoints
#: and the midpoint of the arc.
ArcPoints = namedtuple(
    "ArcPoints", ["a1_x", "a1_y", "a2_x", "a2_y", "middle_x", "middle_y"]
)


def _text_bytes(text):
    data = text.encode("utf-8") if isinstance(text, str) else bytes(text)
    return data, len(data)


class GC:
    """A graphics context: drawing state (colors, line/text style, font).

    Create one with ``GC()``; release it with :meth:`free` or by using it as a
    context manager. Pass it to the ``Image`` drawing methods.
    """

    def __init__(self):
        handle = lib.ICreateGC()
        if not handle:
            raise MemoryError("ICreateGC() failed")
        self._handle = handle

    # -- lifetime ----------------------------------------------------------
    def free(self):
        """Release the underlying graphics context (idempotent)."""
        if getattr(self, "_handle", None):
            lib._IFreeGC(self._handle)
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
            raise ValueError("operation on a freed GC")
        return self._handle

    # -- drawing state -----------------------------------------------------
    def set_foreground(self, color):
        """Set the foreground (drawing) color."""
        check(lib.ISetForeground(self._as_parameter_, int(color)))
        return self

    def set_background(self, color):
        """Set the background color (used by text styles)."""
        check(lib.ISetBackground(self._as_parameter_, int(color)))
        return self

    def set_font(self, font):
        """Set the current drawing font (a :class:`~ilib.font.Font`)."""
        check(lib.ISetFont(self._as_parameter_, font._as_parameter_))
        return self

    def set_line_width(self, width):
        """Set the line width for line/rectangle drawing."""
        check(lib.ISetLineWidth(self._as_parameter_, int(width)))
        return self

    def set_line_style(self, style):
        """Set the line style (a :class:`~ilib.constants.LineStyle`)."""
        check(lib.ISetLineStyle(self._as_parameter_, int(LineStyle(style))))
        return self

    def set_text_style(self, style):
        """Set the text style (a :class:`~ilib.constants.TextStyle`)."""
        check(lib.ISetTextStyle(self._as_parameter_, int(TextStyle(style))))
        return self

    def set_blend_mode(self, mode):
        """Set the pixel compositing mode (a :class:`~ilib.constants.BlendMode`)."""
        check(lib.ISetBlendMode(self._as_parameter_, int(BlendMode(mode))))
        return self

    def set_antialias(self, on=True):
        """Enable or disable anti-aliasing of drawing primitives."""
        check(lib.ISetAntiAlias(self._as_parameter_, 1 if on else 0))
        return self

    # -- text measurement --------------------------------------------------
    def text_width(self, font, text):
        """Return the width in pixels of ``text`` rendered in ``font``."""
        data, length = _text_bytes(text)
        out = ffi.new("unsigned int *")
        check(lib.ITextWidth(self._as_parameter_, font._as_parameter_, data, length, out))
        return int(out[0])

    def text_height(self, font, text):
        """Return the height in pixels of ``text`` rendered in ``font``."""
        data, length = _text_bytes(text)
        out = ffi.new("unsigned int *")
        check(lib.ITextHeight(self._as_parameter_, font._as_parameter_, data, length, out))
        return int(out[0])

    def text_dimensions(self, font, text):
        """Return ``(width, height)`` in pixels of ``text`` rendered in ``font``."""
        data, length = _text_bytes(text)
        w = ffi.new("unsigned int *")
        h = ffi.new("unsigned int *")
        check(
            lib.ITextDimensions(
                self._as_parameter_, font._as_parameter_, data, length, w, h
            )
        )
        return int(w[0]), int(h[0])

    # -- geometry ----------------------------------------------------------
    def arc_properties(self, x, y, r1, r2, a1, a2):
        """Compute the endpoint and midpoint coordinates of an arc.

        Returns an :class:`ArcPoints` namedtuple.
        """
        vals = [ffi.new("int *") for _ in range(6)]
        check(
            lib.IArcProperties(
                self._as_parameter_,
                int(x), int(y), int(r1), int(r2), float(a1), float(a2),
                vals[0], vals[1], vals[2], vals[3], vals[4], vals[5],
            )
        )
        return ArcPoints(*(int(v[0]) for v in vals))
