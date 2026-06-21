# Ilib

[![CI](https://github.com/craigk5n/ilib/actions/workflows/ci.yml/badge.svg)](https://github.com/craigk5n/ilib/actions/workflows/ci.yml)
[![Docs](https://github.com/craigk5n/ilib/actions/workflows/docs.yml/badge.svg)](https://craigk5n.github.io/ilib/)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](COPYING)

Copyright (C) 2001-2016 Craig Knudsen, craig@k5n.us — https://www.k5n.us/Ilib.php

Ilib is a library (and some tools and examples) written in C
that can read, create, manipulate and save images.  It can read and
write PPM, PGM, XPM, BMP, GIF, PNG, JPG, WebP and AVIF image formats.

## Graphics

The drawing API is modeled on a subset of the X11 graphics functions:

- **Shapes:** points, lines, rectangles, polygons, circles, ellipses, and arcs,
  each with outline (`IDraw*`) and filled (`IFill*`) forms, plus flood fill.
- **Curves:** cubic Bézier paths (`IDrawBezier`) and Catmull-Rom splines through
  points (`IDrawSpline`) — handy for line/area charts.
- **Anti-aliasing:** enable per graphics context with `ISetAntiAlias()`; lines,
  curves, circles, ellipses, arcs, and fills all render with smooth edges.
- **Color & alpha:** RGB and named colors; an optional alpha channel
  (`IOPTION_ALPHA`) with source-over compositing (`ISetBlendMode(IBLEND_OVER)`)
  for translucent drawing.
- **Text:** X11 BDF bitmap fonts (208 ship in the distribution), and — when
  built with FreeType — scalable, anti-aliased TrueType/OpenType fonts
  (`ILoadFontFromFileTTF`). Both support text styles and rotation.
- **Filters:** whole-image point operations (ImageMagick-style): greyscale,
  negate, brightness/contrast, gamma, threshold, normalize (auto-contrast),
  sepia, and opacity (`IGreyscale`, `INegate`, `IBrightnessContrast`, `IGamma`,
  `IThreshold`, `INormalize`, `ISepia`, `IOpacity`).
- **Transforms:** geometric operations: flip, flop, rotate (multiples of 90°),
  and crop (`IFlip`, `IFlop`, `IRotate`, `ICrop`).
- **Convolution:** a generic kernel filter (`IConvolve`) plus blur, Gaussian
  blur, sharpen, edge detect, and emboss (`IBlur`, `IGaussianBlur`, `ISharpen`,
  `IEdgeDetect`, `IEmboss`).
- **Resampling:** bilinear resize to any dimensions (`IResize`) and
  arbitrary-angle rotation with a background fill (`IRotateAngle`).
- **Composition:** autocrop a uniform border (`ITrim`), add a solid frame
  (`IBorder`), and combine images — side-by-side/stacked (`IAppend`) or in a
  grid / contact sheet (`IMontage`).
- **Charting:** a small charting layer (`Ichart.h`) builds line, bar and pie
  charts — with title, axes, gridlines, tick/category labels and a legend — and
  renders them to an image. See the `ilib-chart` demo.

Note: This code was originally developed in the late 1990s in "classic"
(K&R) C. It has since been modernized to ISO C (ANSI-style prototypes) and
builds warning-clean under `-Wall -Wextra`.

## API Documentation
The API reference is generated from the annotated public header (`src/Ilib.h`)
with [Doxygen](https://www.doxygen.nl/) and published to GitHub Pages:

- **Online:** https://craigk5n.github.io/ilib/

Build it locally (requires Doxygen) with:

```bash
cmake -B build
cmake --build build --target docs    # output in build/docs/html/
```

For a hands-on introduction see the [tutorial](docs/tutorial.md). The installed
client tools have man pages (`man ilib-index`, `man ilib-font2h`, …).

Note that the drawing API is modeled after a subset of the
[X11 API drawing functions](https://www.x.org/releases/X11R7.6/doc/libX11/specs/libX11/libX11.html#graphics_functions).


## Python bindings

Python bindings live in [`python/`](python/) and use
[cffi](https://cffi.readthedocs.io/) in **ABI mode**: they load an installed
`libilib` shared library at runtime, so installing them needs no C compiler —
only the Ilib C library (built and installed as below).

```python
import ilib

img = ilib.Image(320, 120, ilib.Option.ALPHA)
with ilib.GC() as gc:
    gc.set_antialias(True)
    gc.set_foreground(ilib.named_color("midnightblue"))
    img.fill_rectangle(gc, 0, 0, *img.size)
    gc.set_foreground(ilib.named_color("white"))
    img.draw_circle(gc, 80, 60, 45)
img.save("out.png")
img.free()
```

```bash
pip install ilib-graphics             # from PyPI (once published); import ilib
# or, straight from the repo without cloning:
pip install "git+https://github.com/craigk5n/ilib.git#subdirectory=python"
# or, from a local checkout:
cd python && pip install .
```

`pip` installs only the Python code; the Ilib **C shared library** must also be
installed (or reachable via `ILIB_LIBRARY`) since it is loaded at runtime. See
[`python/README.md`](python/README.md) for the full API, library discovery, how
to run the tests, and the release/publishing process.

## Building

Ilib uses [CMake](https://cmake.org/) (3.16 or newer). It builds with no
external dependencies; GIF, PNG and JPEG support is enabled automatically when
the corresponding development libraries are found.

### Optional dependencies

Install whichever of these you want format support for. FreeType is optional
too and adds scalable, anti-aliased TrueType/OpenType fonts (alongside the
built-in X11 BDF fonts):

- **Debian/Ubuntu:** `sudo apt install cmake libpng-dev libjpeg-dev libgif-dev libwebp-dev libavif-dev libfreetype-dev`
- **Fedora/RHEL:** `sudo dnf install cmake libpng-devel libjpeg-turbo-devel giflib-devel libwebp-devel libavif-devel freetype-devel`
- **macOS (Homebrew):** `brew install cmake giflib libjpeg libpng webp libavif freetype`

Each is auto-detected; a missing one is skipped gracefully.

### Build, test and install

```
cmake -B build                  # add -DILIB_BUILD_TESTS=ON to build the tests
cmake --build build
ctest --test-dir build          # optional; requires -DILIB_BUILD_TESTS=ON
sudo cmake --install build      # installs to /usr/local by default
```

CMake prints a summary of which image formats were enabled. A missing optional
library is simply skipped with a notice rather than failing the build.

### Useful options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_INSTALL_PREFIX` | `/usr/local` | Install location |
| `BUILD_SHARED_LIBS` | `ON` | Build a shared library (`OFF` for static) |
| `ILIB_WITH_PNG` / `ILIB_WITH_JPEG` / `ILIB_WITH_GIF` | `ON` | Toggle a codec even if its library is present |
| `ILIB_BUILD_CLIENTS` / `ILIB_BUILD_EXAMPLES` | `ON` | Build the bundled tools / demos |
| `ILIB_BUILD_TESTS` | `OFF` | Build the CTest test suite |

### Using Ilib from another project

With CMake:

```cmake
find_package(Ilib REQUIRED)
target_link_libraries(your_target PRIVATE Ilib::ilib)
```

Or with pkg-config:

```
cc example.c $(pkg-config --cflags --libs ilib) -o example
```

For **static** linking, `pkg-config --static --libs ilib` adds the transitive
codec/math dependencies (`-lm` plus `-lpng`/`-ljpeg`/`-lgif` for whichever
backends were enabled at build time).

The `examples/` and `clients/` directories show how to use the API:
`ilib-convert` converts images between formats **and** applies a pipeline of
operations (in command-line order), and `ilib-sample` demonstrates drawing
text, lines, and shapes. (All bundled tools install with an `ilib-` prefix.)

```bash
# convert PNG to GIF, desaturate, sharpen, then scale down
ilib-convert in.png out.gif --greyscale --sharpen --resize 320x240
ilib-convert --help          # full list of operations
```

Supported `ilib-convert` operations: `--greyscale`, `--negate`,
`--brightness`, `--contrast`, `--gamma`, `--threshold`, `--flip`, `--flop`,
`--rotate` (with `--background`), `--blur`, `--gaussian-blur`, `--sharpen`,
`--edge`, `--emboss`, `--resize`, `--reduce-colors`, `--normalize`, `--sepia`,
`--opacity`, `--trim`, and `--border`.

The installed `ilib-mogrify` tool applies the same operations to **many files
in place** (batch editing), or — with `--format EXT` — writes converted copies:

```bash
ilib-mogrify --resize 128x128 *.png          # shrink every PNG in place
ilib-mogrify --format gif --reduce-colors 256 *.png   # PNG -> GIF copies
```

### Packaging and releases

Tagged releases (`vX.Y.Z`) publish source and binary tarballs with
`SHA256SUMS` to [GitHub Releases](https://github.com/craigk5n/ilib/releases),
built by `.github/workflows/release.yml`. You can produce the same artifacts
locally from a configured build tree:

```bash
cd build
cpack -G TGZ                                  # binary tarball
cpack --config CPackSourceConfig.cmake        # source tarball
# cpack -G DEB                                 # Debian package (on a Debian host)
```

Downstream packaging notes (Homebrew, Debian, vcpkg) live in
[`packaging/README.md`](packaging/README.md).

## Fonts

Ilib draws text using X11 BDF fonts. A few sample fonts ship in `fonts/` and are
installed to `<prefix>/share/ilib/fonts` (query the exact path with
`pkg-config --variable=fontdir ilib`). Pass a `.bdf` path to
`ILoadFontFromFile()`, or compile a font into your program with the
`ilib-font2h` tool. More BDF fonts:

- [Search the web](https://www.google.com/search?q=timR24.bdf)
- [Apple X11 fonts](https://opensource.apple.com/source/X11fonts/X11fonts-10.2/font-adobe-100dpi/font-adobe-100dpi-X11R7.0-1.0.0/)
- Edit or create BDF fonts with any [BDF font editor](https://www.google.com/search?q=bdf+font+editor).

## Contributing

Bug reports and pull requests are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md)
for the build/test workflow and coding conventions, and
[SECURITY.md](SECURITY.md) for reporting vulnerabilities in the image parsers.
Notable changes are recorded in [CHANGELOG.md](CHANGELOG.md).

## License

Ilib is licensed under the GNU General Public License, version 2 only
(`GPL-2.0-only`). See [COPYING](COPYING) for the full text. Each source file
carries an `SPDX-License-Identifier: GPL-2.0-only` tag.
