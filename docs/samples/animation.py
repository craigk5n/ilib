#!/usr/bin/env python3
"""Regenerate the README demo animation (docs/images/anim-demo.gif).

Draws a rotating ring of anti-aliased dots with the drawing API and assembles
the frames into a looping animated GIF. Run from the repo root with the built
library discoverable, e.g.:

    ILIB_LIBRARY=build/src/libilib.so PYTHONPATH=python/src \\
        python3 docs/samples/animation.py
"""
import math

import ilib
from ilib import GC, Animation, Image

OUT = "docs/images/anim-demo.gif"
W = H = 200
DOTS = 8
RING = 62
FRAMES = 24
DELAY_MS = 70

_PALETTE = [
    (0x4E, 0x79, 0xA7), (0xF2, 0x8E, 0x2B), (0x59, 0xA1, 0x4F),
    (0xE1, 0x57, 0x59), (0x76, 0xB7, 0xB2), (0xED, 0xC9, 0x48),
    (0xB0, 0x7A, 0xA1), (0xFF, 0x9D, 0xA7),
]


def main():
    white = ilib.alloc_color(255, 255, 255)
    colors = [ilib.alloc_color(*rgb) for rgb in _PALETTE]
    anim = Animation()

    for f in range(FRAMES):
        img = Image(W, H)
        gc = GC()
        gc.set_antialias(True)
        gc.set_foreground(white)
        img.fill_rectangle(gc, 0, 0, W, H)

        base = 2.0 * math.pi * f / FRAMES
        for i in range(DOTS):
            ang = base + 2.0 * math.pi * i / DOTS
            x = W / 2 + RING * math.cos(ang)
            y = H / 2 + RING * math.sin(ang)
            # Dot size ramps around the ring so it reads as a spinning comet.
            radius = 4 + 11 * i / (DOTS - 1)
            gc.set_foreground(colors[i % len(colors)])
            img.fill_circle(gc, int(x), int(y), int(radius))

        anim.add_frame(img, DELAY_MS)
        img.free()
        gc.free()

    anim.loop_count = 0  # forever
    anim.save(OUT)
    anim.free()
    print("wrote", OUT)


if __name__ == "__main__":
    main()
