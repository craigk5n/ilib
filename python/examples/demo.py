#!/usr/bin/env python3
"""Small drawing demo for the Ilib Python bindings.

Run from the ``python/`` directory after building the C library:

    ILIB_LIBRARY=$PWD/../build/src/libilib.so \\
        PYTHONPATH=src python examples/demo.py demo.png
"""

import sys

import ilib


def main(out_path="demo.png"):
    width, height = 400, 200
    img = ilib.Image(width, height, ilib.Option.ALPHA)

    with ilib.GC() as gc:
        gc.set_antialias(True)

        # Background.
        gc.set_foreground(ilib.named_color("midnightblue"))
        img.fill_rectangle(gc, 0, 0, width, height)

        # A translucent gold disc, composited source-over.
        gc.set_blend_mode(ilib.BlendMode.OVER)
        gc.set_foreground(ilib.alloc_color_alpha(255, 215, 0, 160))
        img.fill_circle(gc, 110, 100, 70)
        gc.set_blend_mode(ilib.BlendMode.REPLACE)

        # A spline "data" line.
        gc.set_foreground(ilib.named_color("springgreen"))
        gc.set_line_width(1)
        points = [(220, 150), (260, 70), (300, 120), (340, 50), (380, 90)]
        img.draw_spline(gc, points)

        # Outline.
        gc.set_foreground(ilib.named_color("white"))
        img.draw_rectangle(gc, 0, 0, width - 1, height - 1)

    img.save(out_path)
    img.free()
    print(f"wrote {out_path}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "demo.png")
