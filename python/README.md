# ilib — Python bindings

Python bindings for [Ilib](https://github.com/craigk5n/ilib), a C library for
reading, creating, manipulating and saving raster images (PPM/PGM/XPM/BMP/GIF/
PNG/JPEG), with an X11-style drawing API: lines, shapes, curves, anti-aliasing,
RGBA alpha compositing, and BDF / TrueType text.

The bindings use [cffi](https://cffi.readthedocs.io/) in **ABI mode**: they load
an installed `libilib` shared library at runtime, so installing this package
needs **no C compiler** — only the Ilib C library itself.

## Requirements

- Python 3.8+
- `cffi`
- The Ilib C shared library (`libilib.so` / `.dylib`) installed, or reachable
  via the `ILIB_LIBRARY` environment variable.

Build and install the C library first (from the repository root):

```bash
cmake -B build
cmake --build build
sudo cmake --install build      # puts libilib on the system library path
```

## Install

```bash
cd python
pip install .
```

## Locating the C library

The loader looks for the shared library in this order:

1. `$ILIB_LIBRARY` — an explicit path or soname. Handy for pointing at an
   uninstalled build tree:
   ```bash
   export ILIB_LIBRARY=$PWD/build/src/libilib.so
   ```
2. `ctypes.util.find_library("ilib")` (the system library path).
3. Conventional filenames (`libilib.so`, `libilib.dylib`, …).

## Example

```python
import ilib

img = ilib.Image(320, 120, ilib.Option.ALPHA)

with ilib.GC() as gc:
    gc.set_antialias(True)
    gc.set_foreground(ilib.named_color("midnightblue"))
    img.fill_rectangle(gc, 0, 0, *img.size)

    gc.set_foreground(ilib.alloc_color_alpha(255, 215, 0, 180))
    gc.set_blend_mode(ilib.BlendMode.OVER)
    img.fill_circle(gc, 80, 60, 45)

    font = ilib.Font.from_file("../fonts/helvR12.bdf")
    gc.set_font(font)
    gc.set_foreground(ilib.named_color("white"))
    img.draw_string(gc, 140, 65, "Hello, Ilib!")
    font.free()

img.save("hello.png")
img.free()
```

`Image`, `GC` and `Font` are also context managers, so the `with` form frees
them automatically.

## Tests

The test suite uses the standard library `unittest` (no extra dependencies):

```bash
cd python
ILIB_LIBRARY=$PWD/../build/src/libilib.so \
  PYTHONPATH=src python -m unittest discover -s tests -v
```

Tests that need optional features (TrueType text) skip themselves when the C
library was built without them.

## License

GPL-2.0-only, matching the Ilib C library.
