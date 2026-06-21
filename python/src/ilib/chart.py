"""The :class:`Chart` wrapper (line, bar and pie charts)."""

from __future__ import annotations

from ._ffi import ffi, lib
from .constants import ChartType
from .errors import IError, IlibError, check
from .image import Image

__all__ = ["Chart"]


class Chart:
    """A chart built on Ilib's drawing API.

    Create one with ``Chart(ChartType.LINE, w, h)``, configure it, add data
    series, then :meth:`render` it to an :class:`~ilib.image.Image`. Release it
    with :meth:`free` or by using it as a context manager.
    """

    def __init__(self, chart_type, width, height):
        handle = lib.ICreateChart(int(ChartType(chart_type)), int(width), int(height))
        if not handle:
            raise MemoryError("ICreateChart() failed")
        self._handle = handle
        # Keep a reference to the font so it outlives the chart (C borrows it).
        self._font = None

    # -- lifetime ----------------------------------------------------------
    def free(self):
        """Release the underlying chart (idempotent)."""
        if getattr(self, "_handle", None):
            lib._IFreeChart(self._handle)
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
            raise ValueError("operation on a freed Chart")
        return self._handle

    # -- configuration -----------------------------------------------------
    def set_title(self, title):
        """Set the chart title (needs a font to be drawn)."""
        check(lib.IChartSetTitle(self._as_parameter_, _opt_str(title)))
        return self

    def set_axis_labels(self, x_label=None, y_label=None):
        """Set the x- and y-axis labels (line/bar charts)."""
        check(lib.IChartSetAxisLabels(self._as_parameter_, _opt_str(x_label),
                                      _opt_str(y_label)))
        return self

    def set_font(self, font):
        """Set the font for title/labels/legend (a :class:`~ilib.font.Font`)."""
        self._font = font  # keep alive; C borrows the handle
        handle = font._as_parameter_ if font is not None else ffi.NULL
        check(lib.IChartSetFont(self._as_parameter_, handle))
        return self

    def set_background(self, color):
        """Set the background fill color."""
        check(lib.IChartSetBackground(self._as_parameter_, int(color)))
        return self

    def set_categories(self, labels):
        """Set the category / slice labels (a sequence of strings)."""
        labels = list(labels)
        # Keep the encoded strings alive for the duration of the call.
        cstrs = [ffi.new("char[]", s.encode("utf-8")) for s in labels]
        arr = ffi.new("char *[]", cstrs)
        check(lib.IChartSetCategories(self._as_parameter_, arr, len(labels)))
        return self

    def set_range(self, ymin, ymax):
        """Fix the value-axis range (default is auto from the data)."""
        check(lib.IChartSetRange(self._as_parameter_, float(ymin), float(ymax)))
        return self

    def set_stacked(self, stacked=True):
        """For a bar chart, stack series instead of grouping them."""
        check(lib.IChartSetStacked(self._as_parameter_, 1 if stacked else 0))
        return self

    def set_log_scale(self, on=True):
        """Use a logarithmic (base-10) value axis (positive data only)."""
        check(lib.IChartSetLogScale(self._as_parameter_, 1 if on else 0))
        return self

    def add_series(self, values, label=None, color=0):
        """Add a data series (a sequence of values) with a legend label/color."""
        values = [float(v) for v in values]
        if not values:
            raise ValueError("a series needs at least one value")
        arr = ffi.new("double[]", values)
        check(lib.IChartAddSeries(self._as_parameter_, _opt_str(label), arr,
                                  len(values), int(color)))
        return self

    def add_xy_series(self, xvalues, yvalues, label=None, color=0):
        """Add an (x, y) series for a scatter chart (both sequences copied)."""
        xs = [float(v) for v in xvalues]
        ys = [float(v) for v in yvalues]
        if not xs or len(xs) != len(ys):
            raise ValueError("xvalues and yvalues must be non-empty and equal length")
        xa = ffi.new("double[]", xs)
        ya = ffi.new("double[]", ys)
        check(lib.IChartAddXYSeries(self._as_parameter_, _opt_str(label), xa, ya,
                                    len(xs), int(color)))
        return self

    # -- render ------------------------------------------------------------
    def render(self):
        """Render the chart to a new :class:`~ilib.image.Image`."""
        handle = lib.IChartRender(self._as_parameter_)
        if handle == ffi.NULL:
            raise IlibError(IError.InvalidChart, "IChartRender failed")
        return Image(0, 0, _handle=handle)


def _opt_str(s):
    return ffi.NULL if s is None else s.encode("utf-8")
