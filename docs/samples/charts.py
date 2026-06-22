#!/usr/bin/env python3
"""Regenerate the README chart showcase (docs/images/charts.png).

Builds one of each chart type with the Python bindings and montages them into a
2x3 grid. Run from the repo root with the built library discoverable, e.g.:

    ILIB_LIBRARY=build/src/libilib.so PYTHONPATH=python/src \\
        python3 docs/samples/charts.py
"""
import os

import ilib
from ilib import Chart, ChartType, Image

OUT = "docs/images/charts.png"
W, H = 420, 280

_TTF = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
]
_PALETTE = [
    (0x4E, 0x79, 0xA7), (0xF2, 0x8E, 0x2B), (0x59, 0xA1, 0x4F),
    (0xE1, 0x57, 0x59), (0x76, 0xB7, 0xB2),
]


def _font():
    for p in _TTF:
        if os.path.exists(p):
            try:
                return ilib.Font.from_file_ttf(p, 11)
            except Exception:
                pass
    return None


def _color(i):
    r, g, b = _PALETTE[i % len(_PALETTE)]
    return ilib.alloc_color(r, g, b)


def _chart(ctype):
    c = Chart(ctype, W, H)
    f = _font()
    if f:
        c.set_font(f)
    return c


def main():
    months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun"]
    charts = []

    line = _chart(ChartType.LINE)
    line.set_title("Line")
    line.set_categories(months)
    line.add_series([3, 5, 4, 7, 6, 9], "north", _color(0))
    line.add_series([2, 3, 5, 4, 8, 7], "south", _color(1))
    charts.append(line.render())

    bar = _chart(ChartType.BAR)
    bar.set_title("Stacked bar")
    bar.set_categories(months)
    bar.set_stacked(True)
    bar.set_value_labels(True)
    bar.add_series([3, 5, 4, 7, 6, 9], "a", _color(0))
    bar.add_series([2, 3, 5, 4, 8, 7], "b", _color(1))
    charts.append(bar.render())

    pie = _chart(ChartType.PIE)
    pie.set_title("Pie")
    pie.set_categories(["Apples", "Pears", "Plums", "Cherries"])
    pie.set_value_labels(True)
    pie.add_series([30, 22, 15, 8], color=_color(0))
    charts.append(pie.render())

    area = _chart(ChartType.AREA)
    area.set_title("Area")
    area.set_categories(months)
    area.add_series([3, 5, 4, 7, 6, 9], "flow", _color(2))
    charts.append(area.render())

    scatter = _chart(ChartType.SCATTER)
    scatter.set_title("Scatter")
    scatter.add_xy_series([1, 2, 3, 4, 5, 6], [2, 5, 4, 9, 7, 11], "obs",
                          _color(3))
    charts.append(scatter.render())

    logc = _chart(ChartType.LINE)
    logc.set_title("Log scale")
    logc.set_categories(months)
    logc.set_log_scale(True)
    logc.add_series([1, 10, 100, 1000, 5000, 20000], "growth", _color(4))
    charts.append(logc.render())

    hbar = _chart(ChartType.HBAR)
    hbar.set_title("Horizontal bar")
    hbar.set_categories(months)
    hbar.set_value_labels(True)
    hbar.set_bar_radius(5)  # rounded bars
    hbar.set_background_gradient(ilib.alloc_color(245, 248, 255),
                                 ilib.alloc_color(214, 226, 246))
    hbar.add_series([3, 5, 4, 7, 6, 9], "n", _color(0))
    hbar.add_series([2, 3, 5, 4, 8, 7], "s", _color(1))
    charts.append(hbar.render())

    donut = _chart(ChartType.DONUT)
    donut.set_title("Donut")
    donut.set_categories(["Apples", "Pears", "Plums", "Cherries"])
    donut.set_value_labels(True)
    donut.add_series([30, 22, 15, 8], color=_color(0))
    charts.append(donut.render())

    white = ilib.alloc_color(255, 255, 255)
    grid = Image.montage(charts, columns=4, spacing=8, background=white)
    grid.save(OUT)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
