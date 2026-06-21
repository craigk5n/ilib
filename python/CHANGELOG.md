# Changelog — ilib-graphics (Python bindings)

All notable changes to the Python bindings are documented here. The format is
based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the
package follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

This changelog tracks the **Python package** (`ilib-graphics`, imported as
`ilib`), which versions independently of the Ilib C library. See the
[top-level CHANGELOG](../CHANGELOG.md) for the C library.

## [Unreleased]

### Added
- Image filter methods mirroring the new C point operations:
  `Image.greyscale()` (alias `grayscale()`), `negate()`,
  `brightness_contrast()`, `gamma()`, and `threshold()`.
- Image transform methods mirroring the new C geometric operations:
  `Image.flip()`, `flop()`, `rotate()`, and `crop()`.
- Convolution / area filter methods: `Image.convolve()` (arbitrary square
  kernel), `blur()`, `gaussian_blur()`, `sharpen()`, `edge_detect()`, and
  `emboss()`.

## [0.1.0] - 2026-06-20

First public release of the cffi bindings.

### Added
- cffi bindings (ABI mode) that load an installed `libilib` shared library at
  runtime — no C compiler needed to install the package, only the C library.
- A Pythonic, object-oriented API over the full drawing and image-I/O surface:
  - `Image` — create / `open` / `save` (format inferred from the extension or
    given explicitly), RGB and RGBA pixels, `duplicate` / `copy_to` /
    `copy_scaled_to`, `reduce_colors`, transparency, comment, and every
    `draw_*` / `fill_*` primitive plus curves (`draw_bezier`, `draw_spline`),
    `flood_fill`, and text.
  - `GC` — foreground / background / font, line width and style, text style,
    blend mode, anti-aliasing, text measurement, and arc geometry.
  - `Font` — X11 BDF (`from_file`) and optional FreeType TrueType
    (`from_file_ttf`).
  - Color helpers (`alloc_color`, `alloc_color_alpha`, `named_color`) and
    `IntEnum` mirrors of the C constants; `IlibError` wraps non-success
    `IError` codes.
- `Image` / `GC` / `Font` are context managers and free their handles on
  `__del__`.
- Library discovery via `$ILIB_LIBRARY`, then `ctypes.util.find_library`, then
  conventional filenames.
- A `unittest` test suite (TrueType tests skip when the C library was built
  without FreeType) and a runnable drawing demo.
- Packaging: `pip`-installable (PyPI / git URL / local), single-sourced version
  (`ilib._version`), and a tag-triggered publish workflow using PyPI Trusted
  Publishing.

[Unreleased]: https://github.com/craigk5n/ilib/compare/python-v0.1.0...HEAD
[0.1.0]: https://github.com/craigk5n/ilib/releases/tag/python-v0.1.0
