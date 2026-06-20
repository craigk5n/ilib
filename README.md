# Ilib
Copyright (C) 2001-2016 Craig Knudsen, craig@k5n.us
http://www.k5n.us/Ilib.php

Ilib is a library (and some tools and examples) written in C
that can read, create, manipulate and save images.  It is capable
of using X11 BDF fonts for drawing text.  That means you get
lots (208, to be exact) of fonts to use.  You can even create your
own if you know how to create an X11 BDF font.  It can read and
write PPM, XPM, GIF, PNG and JPG image format.  It can read (but not
yet write) BMP.

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
client tools have man pages (`man iindex`, `man ifont2h`, …).

Note that the drawing API is modeled after a subset of the
[X11 API drawing functions](https://www.x.org/releases/X11R7.6/doc/libX11/specs/libX11/libX11.html#graphics_functions).


## Perl Module
The Ilib perl module is now included with the distribution.  It is not
compiled or installed by default.  This perl module builds like all other
perl modules.  AFTER installing the rest of Ilib (see instructions below),
do the following:
```
  cd perl
  perl Makefile.PL
  make
  make install
```
(Normally, you will need to be root to install.)


## Building

Ilib uses [CMake](https://cmake.org/) (3.16 or newer). It builds with no
external dependencies; GIF, PNG and JPEG support is enabled automatically when
the corresponding development libraries are found.

### Optional dependencies

Install whichever of these you want format support for:

- **Debian/Ubuntu:** `sudo apt install cmake libpng-dev libjpeg-dev libgif-dev`
- **Fedora/RHEL:** `sudo dnf install cmake libpng-devel libjpeg-turbo-devel giflib-devel`
- **macOS (Homebrew):** `brew install cmake giflib libjpeg libpng`

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

The `examples/` and `clients/` directories show how to use the API: `iconvert`
converts images between formats and `isample` demonstrates drawing text, lines,
and shapes.

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
`ILoadFontFromFile()`, or compile a font into your program with the `ifont2h`
tool. More BDF fonts:

- [Search the web](https://www.google.com/search?q=timR24.bdf)
- [Apple X11 fonts](https://opensource.apple.com/source/X11fonts/X11fonts-10.2/font-adobe-100dpi/font-adobe-100dpi-X11R7.0-1.0.0/)
- Edit or create BDF fonts with any [BDF font editor](https://www.google.com/search?q=bdf+font+editor).
