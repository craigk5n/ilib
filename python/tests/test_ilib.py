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
    @staticmethod
    def _row_transitions(img, w, y):
        t = 0
        prev = img.get_pixel(0, y)
        for x in range(1, w):
            cur = img.get_pixel(x, y)
            if cur != prev:
                t += 1
            prev = cur
        return t

    def test_dither_diffuses_more_than_reduce(self):
        w, h = 64, 8
        plain = ilib.Image(w, h)
        dith = ilib.Image(w, h)
        with plain, dith:
            for y in range(h):
                for x in range(w):
                    v = x * 255 // (w - 1)
                    plain.set_pixel(x, y, v, v, v)
                    dith.set_pixel(x, y, v, v, v)
            plain.reduce_colors(2)
            dith.dither(2)
            self.assertGreater(self._row_transitions(dith, w, h // 2),
                               self._row_transitions(plain, w, h // 2) + 3)

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

    def test_normalize(self):
        with ilib.Image(4, 4) as img:
            for y in range(4):
                for x in range(4):
                    img.set_pixel(x, y, 100, 100, 100)
            img.set_pixel(1, 0, 150, 150, 150)
            img.normalize()
            self.assertEqual(img.get_pixel(0, 0)[0], 0)
            self.assertEqual(img.get_pixel(1, 0)[0], 255)

    def test_sepia(self):
        with ilib.Image(4, 4) as img:
            for y in range(4):
                for x in range(4):
                    img.set_pixel(x, y, 128, 128, 128)
            img.sepia()
            r, g, b = img.get_pixel(0, 0)
            self.assertGreaterEqual(r, g)
            self.assertGreaterEqual(g, b)
            self.assertGreater(r, b)

    def test_opacity(self):
        with ilib.Image(4, 4, ilib.Option.ALPHA) as img:
            img.set_pixel_alpha(0, 0, 10, 20, 30, 200)
            img.opacity(0.5)
            self.assertEqual(img.get_pixel_alpha(0, 0)[3], 100)
            with self.assertRaises(ilib.IlibError) as ctx:
                img.opacity(-1.0)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)

    def test_chaining(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(0, 0, 100, 150, 200)
            # methods return self, so they chain
            self.assertIs(img.negate().greyscale(), img)


class Transforms(unittest.TestCase):
    def test_auto_orient(self):
        with ilib.Image(4, 2) as img:
            img.set_pixel(0, 0, 255, 0, 0)
            self.assertEqual(img.orientation, 1)
            img.auto_orient(6)  # rotate 90 CW
            self.assertEqual(img.size, (2, 4))
            self.assertEqual(img.get_pixel(1, 0), (255, 0, 0))

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


class Convolution(unittest.TestCase):
    def _grey(self, w, h, v):
        img = ilib.Image(w, h)
        for y in range(h):
            for x in range(w):
                img.set_pixel(x, y, v, v, v)
        return img

    def test_blur_spreads(self):
        with ilib.Image(5, 5) as img:
            for y in range(5):
                for x in range(5):
                    img.set_pixel(x, y, 0, 0, 0)
            img.set_pixel(2, 2, 255, 255, 255)
            img.blur(1)
            self.assertTrue(20 < img.get_pixel(2, 2)[0] < 40)
            self.assertGreater(img.get_pixel(2, 1)[0], 0)

    def test_gaussian_blur_rejects_bad_sigma(self):
        with ilib.Image(5, 5) as img:
            with self.assertRaises(ilib.IlibError) as ctx:
                img.gaussian_blur(0.0)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)

    def test_sharpen_flat_unchanged(self):
        with self._grey(5, 5, 120) as img:
            img.sharpen()
            self.assertEqual(img.get_pixel(2, 2)[0], 120)

    def test_edge_detect_flat_black(self):
        with self._grey(5, 5, 130) as img:
            img.edge_detect()
            self.assertEqual(img.get_pixel(2, 2)[0], 0)

    def test_emboss_flat_mid_grey(self):
        with self._grey(5, 5, 90) as img:
            img.emboss()
            self.assertEqual(img.get_pixel(2, 2)[0], 128)

    def test_convolve_identity(self):
        with ilib.Image(4, 4) as img:
            img.set_pixel(1, 2, 77, 0, 0)
            img.convolve([[0, 0, 0], [0, 1, 0], [0, 0, 0]], divisor=1.0)
            self.assertEqual(img.get_pixel(1, 2)[0], 77)

    def test_convolve_rejects_non_square(self):
        with ilib.Image(4, 4) as img:
            with self.assertRaises(ValueError):
                img.convolve([[1, 0], [0, 1, 0]])


class Resampling(unittest.TestCase):
    def test_resize_dims(self):
        with ilib.Image(4, 4) as img:
            img.resize(8, 2)
            self.assertEqual(img.size, (8, 2))

    def test_resize_preserves_solid(self):
        with ilib.Image(3, 3) as img:
            for y in range(3):
                for x in range(3):
                    img.set_pixel(x, y, 10, 20, 30)
            img.resize(7, 5)
            self.assertEqual(img.get_pixel(3, 2), (10, 20, 30))

    def test_resize_area_averages_checkerboard(self):
        with ilib.Image(8, 8) as img:
            for y in range(8):
                for x in range(8):
                    v = 255 if (x + y) % 2 == 0 else 0
                    img.set_pixel(x, y, v, v, v)
            img.resize(1, 1, ilib.ResizeFilter.AREA)
            r, _, _ = img.get_pixel(0, 0)
            self.assertTrue(118 < r < 138)  # mid-grey, not an extreme

    def test_resize_rejects_zero(self):
        with ilib.Image(4, 4) as img:
            with self.assertRaises(ilib.IlibError) as ctx:
                img.resize(0, 4)
            self.assertEqual(ctx.exception.code, ilib.IError.InvalidArgument)

    def test_rotate_angle_zero_identity(self):
        with ilib.Image(4, 3) as img:
            img.set_pixel(1, 1, 123, 0, 0)
            img.rotate_angle(0, ilib.named_color("black"))
            self.assertEqual(img.size, (4, 3))
            self.assertEqual(img.get_pixel(1, 1)[0], 123)

    def test_rotate_angle_45_grows_and_fills(self):
        with ilib.Image(3, 3) as img:
            green = ilib.named_color("green")
            img.rotate_angle(45, green)
            self.assertGreater(img.size[0], 3)
            self.assertEqual(img.get_pixel(0, 0), (0, 255, 0))


class Composition(unittest.TestCase):
    def test_trim(self):
        with ilib.Image(10, 10) as img:  # white background
            for y in range(3, 7):
                for x in range(3, 7):
                    img.set_pixel(x, y, 255, 0, 0)
            img.trim()
            self.assertEqual(img.size, (4, 4))
            self.assertEqual(img.get_pixel(0, 0), (255, 0, 0))

    def test_border(self):
        with ilib.Image(4, 4) as img:
            for y in range(4):
                for x in range(4):
                    img.set_pixel(x, y, 200, 0, 0)
            img.border(2, ilib.named_color("black"))
            self.assertEqual(img.size, (8, 8))
            self.assertEqual(img.get_pixel(0, 0), (0, 0, 0))
            self.assertEqual(img.get_pixel(2, 2), (200, 0, 0))

    def test_append(self):
        a = ilib.Image(3, 2)
        b = ilib.Image(2, 4)
        try:
            out = ilib.Image.append([a, b], horizontal=True,
                                    background=ilib.named_color("green"))
            try:
                self.assertEqual(out.size, (5, 4))
            finally:
                out.free()
        finally:
            a.free()
            b.free()

    def test_montage(self):
        imgs = [ilib.Image(2, 2) for _ in range(3)]
        try:
            out = ilib.Image.montage(imgs, columns=2, spacing=1,
                                     background=ilib.named_color("green"))
            try:
                self.assertEqual(out.size, (7, 7))
            finally:
                out.free()
        finally:
            for im in imgs:
                im.free()


class Charting(unittest.TestCase):
    def test_line_chart(self):
        with ilib.Chart(ilib.ChartType.LINE, 200, 150) as c:
            c.add_series([1, 3, 2, 5, 4], label="a",
                         color=ilib.alloc_color(200, 0, 0))
            img = c.render()
            try:
                self.assertEqual(img.size, (200, 150))
                # something was drawn (not all white)
                drawn = any(
                    img.get_pixel(x, y) != (255, 255, 255)
                    for y in range(0, 150, 7)
                    for x in range(0, 200, 7)
                )
                self.assertTrue(drawn)
            finally:
                img.free()

    def test_pie_chart(self):
        with ilib.Chart(ilib.ChartType.PIE, 160, 160) as c:
            c.set_categories(["x", "y", "z"])
            c.add_series([30, 50, 20])
            img = c.render()
            try:
                self.assertNotEqual(img.get_pixel(80, 80), (255, 255, 255))
            finally:
                img.free()

    def test_area_chart(self):
        with ilib.Chart(ilib.ChartType.AREA, 200, 150) as c:
            c.add_series([1, 3, 2, 5, 4], label="a",
                         color=ilib.alloc_color(80, 140, 200))
            img = c.render()
            try:
                self.assertEqual(img.size, (200, 150))
            finally:
                img.free()

    def test_scatter_chart(self):
        with ilib.Chart(ilib.ChartType.SCATTER, 200, 150) as c:
            c.add_xy_series([1, 4, 2, 8], [2, 5, 3, 7], label="pts",
                            color=ilib.alloc_color(200, 0, 0))
            img = c.render()
            try:
                self.assertEqual(img.size, (200, 150))
            finally:
                img.free()

    def test_hbar_chart(self):
        with ilib.Chart(ilib.ChartType.HBAR, 240, 160) as c:
            c.set_categories(["x", "y", "z"])
            c.add_series([2, 4, 3], label="a", color=ilib.alloc_color(50, 100, 200))
            c.add_series([1, 5, 2], label="b", color=ilib.alloc_color(200, 100, 50))
            img = c.render()
            try:
                self.assertEqual(img.size, (240, 160))
            finally:
                img.free()

    def test_donut_chart(self):
        with ilib.Chart(ilib.ChartType.DONUT, 160, 160) as c:
            c.set_categories(["x", "y", "z"])
            c.add_series([30, 50, 20])
            img = c.render()
            try:
                # The centre is the background hole (white).
                self.assertEqual(img.get_pixel(80, 80), (255, 255, 255))
            finally:
                img.free()

    def test_display_options(self):
        with ilib.Chart(ilib.ChartType.LINE, 160, 120) as c:
            c.set_markers(False).set_grid(False).set_legend(False)
            c.add_series([1, 2, 3], label="s", color=ilib.alloc_color(10, 20, 30))
            img = c.render()
            try:
                self.assertEqual(img.size, (160, 120))
            finally:
                img.free()

    def test_stacked_bar(self):
        with ilib.Chart(ilib.ChartType.BAR, 220, 160) as c:
            c.set_stacked(True)
            c.add_series([2, 4, 3], label="a", color=ilib.alloc_color(50, 100, 200))
            c.add_series([1, 2, 5], label="b", color=ilib.alloc_color(200, 100, 50))
            img = c.render()
            try:
                self.assertEqual(img.size, (220, 160))
            finally:
                img.free()

    def test_log_scale(self):
        with ilib.Chart(ilib.ChartType.LINE, 220, 160) as c:
            c.set_log_scale(True)
            c.add_series([1, 10, 100, 1000], label="exp",
                         color=ilib.named_color("black"))
            img = c.render()
            try:
                self.assertEqual(img.size, (220, 160))
            finally:
                img.free()

    def test_value_labels(self):
        with ilib.Chart(ilib.ChartType.BAR, 220, 160) as c:
            c.set_value_labels(True)
            c.add_series([2, 4, 3], label="a", color=ilib.alloc_color(50, 100, 200))
            img = c.render()
            try:
                self.assertEqual(img.size, (220, 160))
            finally:
                img.free()

    def test_bar_chart_and_config(self):
        with ilib.Chart(ilib.ChartType.BAR, 220, 160) as c:
            c.set_title("T").set_categories(["Q1", "Q2", "Q3"]).set_range(0, 10)
            c.add_series([2, 4, 3], label="u", color=ilib.named_color("green"))
            img = c.render()
            try:
                self.assertEqual(img.size, (220, 160))
            finally:
                img.free()


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

    def test_webp_roundtrip_or_skip(self):
        unsupported = {
            ilib.IError.NoWEBPSupport,
            ilib.IError.WEBPError,
            ilib.IError.InvalidFormat,
            ilib.IError.FunctionNotImplemented,
        }
        with ilib.Image(16, 16) as img:
            for y in range(16):
                for x in range(16):
                    img.set_pixel(x, y, 200, 100, 50)
            with tempfile.TemporaryDirectory() as d:
                path = os.path.join(d, "x.webp")
                try:
                    img.save(path)
                except ilib.IlibError as exc:
                    if exc.code in unsupported:
                        self.skipTest("library built without WebP")
                    raise
                with ilib.Image.open(path) as back:
                    self.assertEqual(back.size, (16, 16))
                    r, g, b = back.get_pixel(8, 8)
                    # lossy, but a solid colour comes back close
                    self.assertLess(abs(r - 200), 12)
                    self.assertLess(abs(g - 100), 12)
                    self.assertLess(abs(b - 50), 12)

    def test_avif_roundtrip_or_skip(self):
        unsupported = {
            ilib.IError.NoAVIFSupport,
            ilib.IError.AVIFError,
            ilib.IError.InvalidFormat,
            ilib.IError.FunctionNotImplemented,
        }
        with ilib.Image(16, 16) as img:
            for y in range(16):
                for x in range(16):
                    img.set_pixel(x, y, 200, 100, 50)
            with tempfile.TemporaryDirectory() as d:
                path = os.path.join(d, "x.avif")
                try:
                    img.save(path)
                except ilib.IlibError as exc:
                    if exc.code in unsupported:
                        self.skipTest("library built without AVIF")
                    raise
                with ilib.Image.open(path) as back:
                    self.assertEqual(back.size, (16, 16))
                    r, g, b = back.get_pixel(8, 8)
                    self.assertLess(abs(r - 200), 16)
                    self.assertLess(abs(g - 100), 16)
                    self.assertLess(abs(b - 50), 16)

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


class TextLayout(unittest.TestCase):
    def test_word_wrap(self):
        path = find_bdf_font()
        if not path:
            self.skipTest("no BDF font found")
        font = ilib.Font.from_file(path)
        try:
            with ilib.GC() as gc:
                s = "the quick brown fox jumps over the lazy dog"
                _, _, lines1 = gc.text_box_dimensions(font, s, 0)
                w_wrap, h_wrap, lines_wrap = gc.text_box_dimensions(font, s, 60)
                self.assertEqual(lines1, 1)
                self.assertGreater(lines_wrap, 1)
                with ilib.Image(80, h_wrap + 4) as img:
                    gc.set_font(font)
                    gc.set_foreground(ilib.alloc_color(0, 0, 0))
                    img.draw_text(gc, 2, 8, s, max_width=60,
                                  align=ilib.HAlign.LEFT)
        finally:
            font.free()


class TextAlign(unittest.TestCase):
    def test_text_coordinates(self):
        path = find_bdf_font()
        if not path:
            self.skipTest("no BDF font found")
        font = ilib.Font.from_file(path)
        try:
            with ilib.GC() as gc:
                w, h = gc.text_dimensions(font, "Hello")
                x, y = gc.text_coordinates(font, "Hello", 100, 100,
                                           ilib.HAlign.CENTER, ilib.VAlign.MIDDLE)
                self.assertEqual(x, 100 - w // 2)
                self.assertEqual(y, 100 + h // 2)
                x, y = gc.text_coordinates(font, "Hello", 100, 100)
                self.assertEqual((x, y), (100, 100))  # left/bottom default
        finally:
            font.free()


class Geometry(unittest.TestCase):
    def test_arc_properties(self):
        with ilib.GC() as gc:
            pts = gc.arc_properties(50, 50, 40, 40, 0.0, 90.0)
            self.assertIsInstance(pts, ilib.ArcPoints)
            # angle 0 endpoint is to the right of center.
            self.assertGreater(pts.a1_x, 50)


class Animations(unittest.TestCase):
    @staticmethod
    def _solid(w, h, r, g, b):
        im = ilib.Image(w, h)
        for y in range(h):
            for x in range(w):
                im.set_pixel(x, y, r, g, b)
        return im

    def test_build_and_query(self):
        with ilib.Animation() as a:
            self.assertEqual(len(a), 0)
            for img, d in ((self._solid(8, 6, 255, 0, 0), 100),
                           (self._solid(8, 6, 0, 0, 255), 200)):
                with img:
                    a.add_frame(img, d)
            a.loop_count = 3
            self.assertEqual(len(a), 2)
            self.assertEqual(a.delay(0), 100)
            self.assertEqual(a.loop_count, 3)
            with a.frame(0) as f:
                self.assertEqual(f.size, (8, 6))

    def test_gif_round_trip(self):
        a = ilib.Animation()
        for img, d in ((self._solid(12, 10, 255, 0, 0), 100),
                       (self._solid(12, 10, 0, 0, 255), 200)):
            with img:
                a.add_frame(img, d)
        a.loop_count = 5
        path = os.path.join(tempfile.gettempdir(), "ilib_pyanim.gif")
        try:
            try:
                a.save(path)
            except ilib.IlibError as e:
                if e.code == ilib.IError.NoGIFSupport:
                    self.skipTest("no giflib")
                raise
            finally:
                a.free()
            b = ilib.Animation.open(path)
            try:
                self.assertEqual(len(b), 2)
                self.assertEqual(b.delay(1), 200)
                self.assertEqual(b.loop_count, 5)
                with b.frame(0) as f:
                    self.assertEqual(f.get_pixel(6, 5), (255, 0, 0))
            finally:
                b.free()
        finally:
            if os.path.exists(path):
                os.unlink(path)


if __name__ == "__main__":
    unittest.main()
