"""Multi-frame animations (animated GIF)."""

from __future__ import annotations

from ._ffi import ffi, lib, libc
from .constants import Option
from .errors import IError, IlibError, check
from .image import Image, _format_for

__all__ = ["Animation"]


class Animation:
    """An ordered list of frames (images) with per-frame delays and a loop count.

    Build one with :meth:`add_frame`, write it with :meth:`save`, or load one
    with :meth:`open`. Release it with :meth:`free` or as a context manager.
    Currently reads and writes animated GIF.
    """

    def __init__(self, _handle=None):
        if _handle is not None:
            self._handle = _handle
            return
        handle = lib.ICreateAnimation()
        if not handle:
            raise MemoryError("ICreateAnimation() failed")
        self._handle = handle

    def free(self):
        """Free the animation and all its frames."""
        if getattr(self, "_handle", None):
            lib._IFreeAnimation(self._handle)
            self._handle = None

    def __del__(self):
        try:
            self.free()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.free()

    @property
    def _as_parameter_(self):
        if not getattr(self, "_handle", None):
            raise IlibError(IError.InvalidAnimation, "animation already freed")
        return self._handle

    # -- building ----------------------------------------------------------
    def add_frame(self, image, delay_ms=100):
        """Append a copy of ``image`` as the next frame, shown for ``delay_ms``."""
        check(lib.IAddAnimationFrame(self._as_parameter_, image._as_parameter_,
                                     int(delay_ms)))
        return self

    @property
    def loop_count(self):
        """Loop count (0 = loop forever)."""
        return int(lib.IAnimationLoopCount(self._as_parameter_))

    @loop_count.setter
    def loop_count(self, n):
        check(lib.ISetAnimationLoopCount(self._as_parameter_, int(n)))

    # -- access ------------------------------------------------------------
    def __len__(self):
        return int(lib.IAnimationFrameCount(self._as_parameter_))

    @property
    def frame_count(self):
        return len(self)

    def frame(self, index):
        """Return frame ``index`` as a new (owned) :class:`~ilib.image.Image`."""
        h = lib.IAnimationFrame(self._as_parameter_, int(index))
        if h == ffi.NULL:
            raise IndexError(index)
        out = ffi.new("IImage *")
        check(lib.IDuplicateImage(h, out))
        return Image(0, 0, _handle=out[0])

    def delay(self, index):
        """Display delay of frame ``index`` in milliseconds."""
        return int(lib.IAnimationFrameDelay(self._as_parameter_, int(index)))

    # -- I/O ---------------------------------------------------------------
    def save(self, path, fmt=None, options=Option.NONE):
        """Write the animation to ``path`` (format inferred from the extension)."""
        format_ = _format_for(path, fmt)
        fp = libc.fopen(str(path).encode("utf-8"), b"wb")
        if not fp:
            raise IlibError(IError.NoSuchFile, f"cannot open {path!r} for writing")
        try:
            check(lib.IWriteAnimationFile(fp, self._as_parameter_,
                                          int(format_), int(options)))
        finally:
            libc.fclose(fp)

    @classmethod
    def open(cls, path, fmt=None):
        """Read an animation from ``path`` (format inferred from the extension)."""
        format_ = _format_for(path, fmt)
        fp = libc.fopen(str(path).encode("utf-8"), b"rb")
        if not fp:
            raise IlibError(IError.NoSuchFile, f"cannot open {path!r} for reading")
        try:
            handle = lib.IReadAnimationFile(fp, int(format_))
        finally:
            libc.fclose(fp)
        if handle == ffi.NULL:
            raise IlibError(IError.GIFError, f"cannot read animation from {path!r}")
        return cls(_handle=handle)
