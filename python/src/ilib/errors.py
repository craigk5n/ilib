"""Error codes and the :class:`IlibError` exception."""

from __future__ import annotations

from enum import IntEnum

from ._ffi import ffi, lib


class IError(IntEnum):
    """Mirror of the C ``IError`` enum (see ``Ilib.h``)."""

    NoError = 0
    InvalidImage = 1
    InvalidGC = 2
    InvalidColor = 3
    NoTransparentColor = 4
    InvalidFont = 5
    FunctionNotImplemented = 6
    InvalidFormat = 7
    FileInvalid = 8
    ErrorWriting = 9
    NoFontSet = 10
    NoSuchFont = 11
    NoSuchFile = 12
    FontError = 13
    InvalidEscapeSequence = 14
    InvalidArgument = 15
    InvalidColorName = 16
    GIFError = 17
    NoGIFSupport = 18
    PNGError = 19
    NoPNGSupport = 20
    InvalidPolygon = 21
    WEBPError = 22
    NoWEBPSupport = 23
    AVIFError = 24
    NoAVIFSupport = 25
    InvalidChart = 26


def error_string(code):
    """Return the human-readable message for an ``IError`` code."""
    return ffi.string(lib.IErrorString(int(code))).decode("utf-8", "replace")


class IlibError(Exception):
    """Raised when an Ilib function returns a non-success error code.

    The numeric code is available as :attr:`code` (an :class:`IError`).
    """

    def __init__(self, code, message=None):
        try:
            self.code = IError(int(code))
        except ValueError:
            self.code = int(code)
        if message is None:
            message = error_string(code)
        self.message = message
        super().__init__(f"{message} (IError {int(code)})")


def check(code):
    """Raise :class:`IlibError` if ``code`` is not ``INoError``; else return it."""
    if code != IError.NoError:
        raise IlibError(code)
    return code
