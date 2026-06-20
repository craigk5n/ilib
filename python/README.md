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

> **Prerequisite:** these bindings load the Ilib **C shared library** at
> runtime. `pip install` only installs the Python code — you must also have
> `libilib` installed (see above), or point `ILIB_LIBRARY` at it. Without it,
> `import ilib` raises a clear "could not locate the Ilib shared library" error.

From PyPI (once published) — the distribution is named **`ilib-graphics`**
(the bare `ilib` name was already taken); the import name is still `ilib`:

```bash
pip install ilib-graphics
```
```python
import ilib  # the import name is unchanged
```

Straight from the repository, without cloning (pip understands the
`subdirectory` fragment):

```bash
pip install "git+https://github.com/craigk5n/ilib.git#subdirectory=python"
```

From a local checkout:

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

## Releasing (maintainers)

The package version is single-sourced in
[`src/ilib/_version.py`](src/ilib/_version.py). Publishing is automated by
`.github/workflows/python-publish.yml` using **PyPI Trusted Publishing** (OIDC),
so no API tokens are stored.

To cut a release:

1. Bump `__version__` in `src/ilib/_version.py` and commit.
2. Tag and push:
   ```bash
   git tag python-v0.1.0
   git push origin python-v0.1.0
   ```
   The `python-v*` tag triggers a build (sdist + wheel) and publishes to PyPI.
   (The C library uses `v*` tags, kept separate from `python-v*`.)

For a dry run, trigger the workflow manually (`workflow_dispatch`) with the
`testpypi` target to publish to TestPyPI first.

One-time setup on the index side: create the project and register this repo's
`python-publish.yml` workflow as a trusted publisher under the `pypi` (and
`testpypi`) environments. See the comments at the top of the workflow file.

## Changelog

Notable changes to the Python package are recorded in
[CHANGELOG.md](CHANGELOG.md) (the package versions independently of the C
library).

## License

GPL-2.0-only, matching the Ilib C library.
