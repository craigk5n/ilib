#!/usr/bin/env python3
"""Regenerate the README/gallery demo animation (docs/images/anim-demo.gif).

Draws a rotating ring of dots over a smooth two-hue gradient and assembles the
frames into a looping animated GIF. The gradient has far more than 256 colors,
so the GIF writer must quantize it -- the animation is saved with dithering so
the backdrop stays smooth instead of banding. Run from the repo root with the
built library discoverable, e.g.:

    ILIB_LIBRARY=build/src/libilib.so PYTHONPATH=python/src \\
        python3 docs/samples/animation.py
"""
import math

import ilib
from ilib import GC, Animation, Image, Option

OUT = "docs/images/anim-demo.gif"
W = H = 200
DOTS = 8
RING = 62
FRAMES = 24
DELAY_MS = 70


def _lerp(a, b, t):
    return int(a + (b - a) * t + 0.5)


def _gradient():
    """A diagonal deep-blue -> warm-orange gradient (many colors)."""
    img = Image(W, H)
    for y in range(H):
        for x in range(W):
            t = (x + y) / (W + H - 2)
            img.set_pixel(x, y, _lerp(36, 242, t), _lerp(54, 150, t),
                          _lerp(150, 44, t))
    return img


def main():
    white = ilib.alloc_color(255, 255, 255)
    base = _gradient()
    anim = Animation()

    for f in range(FRAMES):
        img = base.duplicate()
        gc = GC()
        gc.set_antialias(True)
        base_ang = 2.0 * math.pi * f / FRAMES
        for i in range(DOTS):
            ang = base_ang + 2.0 * math.pi * i / DOTS
            x = W / 2 + RING * math.cos(ang)
            y = H / 2 + RING * math.sin(ang)
            # Dot size ramps around the ring so it reads as a spinning comet.
            radius = 4 + 11 * i / (DOTS - 1)
            gc.set_foreground(white)
            img.fill_circle(gc, int(x), int(y), int(radius))
        anim.add_frame(img, DELAY_MS)
        img.free()
        gc.free()

    base.free()
    anim.loop_count = 0  # forever
    anim.save(OUT, options=Option.DITHER)  # dither the quantized gradient
    anim.free()
    print("wrote", OUT)


if __name__ == "__main__":
    main()
