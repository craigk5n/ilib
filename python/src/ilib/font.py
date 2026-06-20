"""The :class:`Font` wrapper (X11 BDF and, optionally, FreeType TrueType)."""

from __future__ import annotations

import os

from ._ffi import ffi, lib
from .errors import check

__all__ = ["Font"]


class Font:
    """A drawing font. Construct via :meth:`from_file` or :meth:`from_file_ttf`."""

    def __init__(self, handle, name):
        self._handle = handle
        self.name = name

    @classmethod
    def from_file(cls, path, name=None):
        """Load an X11 BDF font from ``path``.

        ``name`` is the reference name (defaults to the file's base name).
        """
        if name is None:
            name = os.path.basename(path)
        out = ffi.new("IFont *")
        check(
            lib.ILoadFontFromFile(
                name.encode("utf-8"), str(path).encode("utf-8"), out
            )
        )
        return cls(out[0], name)

    @classmethod
    def from_file_ttf(cls, path, pixel_size, name=None):
        """Load a scalable TrueType/OpenType font at ``pixel_size`` (FreeType).

        Raises :class:`~ilib.errors.IlibError` (``FunctionNotImplemented``) if
        the C library was built without FreeType support.
        """
        if name is None:
            name = os.path.basename(path)
        out = ffi.new("IFont *")
        check(
            lib.ILoadFontFromFileTTF(
                name.encode("utf-8"),
                str(path).encode("utf-8"),
                int(pixel_size),
                out,
            )
        )
        return cls(out[0], name)

    # -- lifetime ----------------------------------------------------------
    def free(self):
        """Release the underlying font (idempotent)."""
        if getattr(self, "_handle", None):
            lib._IFreeFont(self._handle)
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
            raise ValueError("operation on a freed Font")
        return self._handle

    # -- queries -----------------------------------------------------------
    @property
    def size(self):
        """The pixel height of the font."""
        out = ffi.new("unsigned int *")
        check(lib.IFontSize(self._as_parameter_, out))
        return int(out[0])
