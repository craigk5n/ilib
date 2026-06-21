"""Tests for the Ilib Python bindings.

Run with the C library reachable (see python/README.md), e.g.::

    ILIB_LIBRARY=$PWD/../build/src/libilib.so \\
        PYTHONPATH=src python -m unittest discover -s tests -v
"""

import glob
import os
import tempfile
import unittest

import ilib

_HERE = os.path.dirname(os.path.abspath(__file__))
_FONT_DIRS = [
    os.path.join(_HERE, "..", "..", "fonts"),
    "/usr/share/fonts",
]


def find_bdf_font():
    for d in _FONT_DIRS:
        matches = sorted(glob.glob(os.path.join(d, "*.bdf")))
        if matches:
            return matches[0]
    return None


def find_ttf_font():
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    ]
    for path in candidates:
        if os.path.exists(path):
            return path
    return None


class ImageBasics(unittest.TestCase):
    def test_create_and_size(self):
        img = ilib.Image(64, 48)
        try:
            self.assertEqual(img.size, (64, 48))
            self.assertEqual(img.width, 64)
            self.assertEqual(img.height, 48)
        finally:
            img.free()

    def test_context_manager_frees(self):
        with ilib.Image(8, 8) as img:
            self.assertEqual(img.width, 8)
        # Using a freed image raises.
        with self.assertRaises(ValueError):
            _ = img.width

    def test_pixel_roundtrip(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(1, 2, 10, 20, 30)
            self.assertEqual(img.get_pixel(1, 2), (10, 20, 30))

    def test_pixel_out_of_range_raises(self):
        with ilib.Image(4, 4) as img:
            with self.assertRaises(ilib.IlibError) as ctx:
                img.set_pixel(99, 99, 0, 0, 0)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)

    def test_alpha_pixel_roundtrip(self):
        with ilib.Image(4, 4, ilib.Option.ALPHA) as img:
            img.set_pixel_alpha(0, 0, 1, 2, 3, 128)
            self.assertEqual(img.get_pixel_alpha(0, 0), (1, 2, 3, 128))

    def test_duplicate_is_independent(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 255, 0, 0)
            dup = img.duplicate()
            try:
                self.assertEqual(dup.get_pixel(0, 0), (255, 0, 0))
                dup.set_pixel(0, 0, 0, 255, 0)
                self.assertEqual(img.get_pixel(0, 0), (255, 0, 0))
            finally:
                dup.free()


class Colors(unittest.TestCase):
    def test_alloc_color(self):
        c = ilib.alloc_color(10, 20, 30)
        self.assertIsInstance(c, int)

    def test_named_color(self):
        c = ilib.named_color("blue")
        self.assertIsInstance(c, int)
        self.assertNotEqual(c, ilib.BLACK)

    def test_named_color_invalid(self):
        with self.assertRaises(ilib.IlibError) as ctx:
            ilib.named_color("notacolor")
        self.assertEqual(ctx.exception.code, ilib.IError.InvalidColorName)

    def test_channel_validation(self):
        with self.assertRaises(ValueError):
            ilib.alloc_color(256, 0, 0)


class Drawing(unittest.TestCase):
    def setUp(self):
        self.img = ilib.Image(40, 40)
        self.gc = ilib.GC()
        self.fg = ilib.named_color("red")
        self.gc.set_foreground(self.fg)

    def tearDown(self):
        self.gc.free()
        self.img.free()

    def test_fill_rectangle(self):
        self.img.fill_rectangle(self.gc, 0, 0, 40, 40)
        self.assertEqual(self.img.get_pixel(20, 20), (255, 0, 0))

    def test_draw_line_marks_pixels(self):
        self.img.draw_line(self.gc, 0, 0, 39, 0)
        self.assertEqual(self.img.get_pixel(20, 0), (255, 0, 0))

    def test_draw_point(self):
        self.img.draw_point(self.gc, 5, 5)
        self.assertEqual(self.img.get_pixel(5, 5), (255, 0, 0))

    def test_polygon_and_fill(self):
        pts = [(5, 5), (35, 5), (20, 35)]
        self.img.fill_polygon(self.gc, pts)
        self.assertEqual(self.img.get_pixel(20, 10), (255, 0, 0))

    def test_bezier(self):
        # 4 points = one cubic segment.
        self.img.draw_bezier(self.gc, [(0, 39), (10, 0), (30, 0), (39, 39)])

    def test_bezier_bad_count_raises(self):
        with self.assertRaises(ilib.IlibError):
            self.img.draw_bezier(self.gc, [(0, 0), (10, 10), (20, 0)])

    def test_spline(self):
        self.img.draw_spline(self.gc, [(0, 20), (10, 5), (20, 30), (39, 10)])

    def test_circle_and_ellipse(self):
        self.img.draw_circle(self.gc, 20, 20, 10)
        self.img.draw_ellipse(self.gc, 20, 20, 12, 6)
        self.img.fill_circle(self.gc, 20, 20, 3)
        self.assertEqual(self.img.get_pixel(20, 20), (255, 0, 0))

    def test_arc(self):
        self.img.draw_arc(self.gc, 20, 20, 10, 10, 0.0, 90.0)
        self.img.fill_arc(self.gc, 20, 20, 8, 8, 0.0, 90.0)

    def test_flood_fill(self):
        self.img.fill_rectangle(self.gc, 0, 0, 40, 40)
        self.gc.set_foreground(ilib.named_color("green"))
        self.img.flood_fill(self.gc, 20, 20)
        self.assertEqual(self.img.get_pixel(20, 20), (0, 255, 0))

    def test_antialias_blends_edges(self):
        self.gc.set_antialias(True)
        self.img.draw_line(self.gc, 0, 0, 39, 25)
        # The image starts white; a red AA line composites over it, so the red
        # channel stays 255 while green/blue take partial (0 < v < 255) values
        # along the smoothed edge.
        blended = False
        for y in range(40):
            for x in range(40):
                _r, g, _b = self.img.get_pixel(x, y)
                if 0 < g < 255:
                    blended = True
                    break
            if blended:
                break
        self.assertTrue(blended, "expected anti-aliased (partial) coverage")

    def test_blend_mode_over(self):
        # Opaque red background, then 50% white over it -> a lighter red.
        self.img.fill_rectangle(self.gc, 0, 0, 40, 40)
        self.gc.set_blend_mode(ilib.BlendMode.OVER)
        self.gc.set_foreground(ilib.alloc_color_alpha(255, 255, 255, 128))
        self.img.fill_rectangle(self.gc, 0, 0, 40, 40)
        r, g, b = self.img.get_pixel(20, 20)
        self.assertGreater(g, 100)  # white bled in
        self.assertLess(g, 255)
        self.assertGreater(r, g)  # still reddish


class Filters(unittest.TestCase):
    def test_greyscale(self):
        with ilib.Image(4, 4) as img:
            for y in range(4):
                for x in range(4):
                    img.set_pixel(x, y, 255, 0, 0)
            img.greyscale()
            r, g, b = img.get_pixel(1, 1)
            self.assertEqual(r, g)
            self.assertEqual(g, b)
            self.assertTrue(70 < r < 82)

    def test_negate(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 10, 20, 30)
            img.negate()
            self.assertEqual(img.get_pixel(0, 0), (245, 235, 225))

    def test_brightness(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 100, 100, 100)
            img.brightness_contrast(50, 0)
            r, _, _ = img.get_pixel(0, 0)
            self.assertGreater(r, 100)

    def test_gamma_identity_and_reject(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 64, 64, 64)
            img.gamma(1.0)
            self.assertEqual(img.get_pixel(0, 0)[0], 64)
            with self.assertRaises(ilib.IlibError) as ctx:
                img.gamma(0.0)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)

    def test_threshold(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 50, 50, 50)
            img.set_pixel(1, 0, 200, 200, 200)
            img.threshold(128)
            self.assertEqual(img.get_pixel(0, 0), (0, 0, 0))
            self.assertEqual(img.get_pixel(1, 0), (255, 255, 255))

    def test_chaining(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 100, 150, 200)
            # methods return self, so they chain
            self.assertIs(img.negate().greyscale(), img)


class Transforms(unittest.TestCase):
    def test_flip(self):
        with ilib.Image(3, 2) as img:
            img.set_pixel(0, 0, 10, 0, 0)
            img.set_pixel(0, 1, 20, 0, 0)
            img.flip()
            self.assertEqual(img.get_pixel(0, 0)[0], 20)
            self.assertEqual(img.get_pixel(0, 1)[0], 10)

    def test_flop(self):
        with ilib.Image(3, 2) as img:
            img.set_pixel(0, 0, 10, 0, 0)
            img.set_pixel(2, 0, 30, 0, 0)
            img.flop()
            self.assertEqual(img.get_pixel(0, 0)[0], 30)
            self.assertEqual(img.get_pixel(2, 0)[0], 10)

    def test_rotate_90_swaps_dims(self):
        with ilib.Image(3, 2) as img:
            img.set_pixel(0, 0, 10, 0, 0)
            img.rotate(90)
            self.assertEqual(img.size, (2, 3))
            self.assertEqual(img.get_pixel(1, 0)[0], 10)  # top-left -> top-right

    def test_rotate_rejects_non_multiple(self):
        with ilib.Image(3, 2) as img:
            with self.assertRaises(ilib.IlibError) as ctx:
                img.rotate(45)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)

    def test_crop(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(1, 1, 99, 0, 0)
            img.crop(1, 1, 2, 2)
            self.assertEqual(img.size, (2, 2))
            self.assertEqual(img.get_pixel(0, 0)[0], 99)

    def test_crop_out_of_bounds(self):
        with ilib.Image(4, 4) as img:
            with self.assertRaises(ilib.IlibError) as ctx:
                img.crop(2, 2, 4, 4)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)


class FileIO(unittest.TestCase):
    def test_ppm_roundtrip(self):
        with ilib.Image(8, 6) as img, ilib.GC() as gc:
            gc.set_foreground(ilib.named_color("red"))
            img.fill_rectangle(gc, 0, 0, 8, 6)
            with tempfile.TemporaryDirectory() as d:
                path = os.path.join(d, "x.ppm")
                img.save(path)
                self.assertTrue(os.path.getsize(path) > 0)
                with ilib.Image.open(path) as loaded:
                    self.assertEqual(loaded.size, (8, 6))
                    self.assertEqual(loaded.get_pixel(4, 3), (255, 0, 0))

    def test_unknown_extension_raises(self):
        with ilib.Image(2, 2) as img:
            with tempfile.TemporaryDirectory() as d:
                with self.assertRaises(ilib.IlibError) as ctx:
                    img.save(os.path.join(d, "x.bogus"))
                self.assertEqual(ctx.exception.code, ilib.IError.InvalidFormat)

    def test_explicit_format(self):
        with ilib.Image(4, 4) as img:
            with tempfile.TemporaryDirectory() as d:
                path = os.path.join(d, "noext")
                img.save(path, fmt=ilib.Format.PPM)
                with ilib.Image.open(path, fmt=ilib.Format.PPM) as loaded:
                    self.assertEqual(loaded.size, (4, 4))


class Text(unittest.TestCase):
    def test_bdf_text(self):
        font_path = find_bdf_font()
        if not font_path:
            self.skipTest("no BDF font found")
        with ilib.Image(160, 40) as img, ilib.GC() as gc:
            font = ilib.Font.from_file(font_path)
            try:
                self.assertGreater(font.size, 0)
                gc.set_font(font)
                gc.set_foreground(ilib.named_color("white"))
                img.fill_rectangle(gc, 0, 0, 160, 40)
                gc.set_foreground(ilib.named_color("black"))
                w, h = gc.text_dimensions(font, "Hi")
                self.assertGreater(w, 0)
                self.assertGreater(h, 0)
                self.assertEqual(gc.text_width(font, "Hi"), w)
                img.draw_string(gc, 2, 20, "Hi")
                # Some black ink should have landed.
                inked = any(
                    img.get_pixel(x, y) == (0, 0, 0)
                    for y in range(40)
                    for x in range(40)
                )
                self.assertTrue(inked, "expected text ink")
            finally:
                font.free()

    def test_ttf_text_or_skip(self):
        ttf = find_ttf_font()
        if not ttf:
            self.skipTest("no TTF font found")
        try:
            font = ilib.Font.from_file_ttf(ttf, 18)
        except ilib.IlibError as exc:
            if exc.code == ilib.IError.FunctionNotImplemented:
                self.skipTest("library built without FreeType")
            raise
        try:
            with ilib.Image(160, 40) as img, ilib.GC() as gc:
                gc.set_font(font)
                gc.set_foreground(ilib.named_color("black"))
                img.draw_string(gc, 2, 30, "Hi")
        finally:
            font.free()


class Geometry(unittest.TestCase):
    def test_arc_properties(self):
        with ilib.GC() as gc:
            pts = gc.arc_properties(50, 50, 40, 40, 0.0, 90.0)
            self.assertIsInstance(pts, ilib.ArcPoints)
            # angle 0 endpoint is to the right of center.
            self.assertGreater(pts.a1_x, 50)


if __name__ == "__main__":
    unittest.main()
