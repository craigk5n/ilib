# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims
to follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

A modernization effort brought the late-1990s codebase up to current practice
without breaking the public API.

### Added
- Smooth curves: `IDrawBezier()` (chained cubic Bezier paths) and
  `IDrawSpline()` (Catmull-Rom spline through a set of points, for line/area
  charts). Both honor the GC line style and anti-aliasing.
- Optional scalable (TrueType/OpenType) fonts via FreeType: `ILoadFontFromFileTTF()`
  loads a font at a pixel size and `IDrawString()` renders it **anti-aliased**.
  Text styles (etched/shadowed), the `IDrawStringRotated()` directions, and
  arbitrary-angle `IDrawStringRotatedAngle()` all apply to TTF text (rotation
  via FreeType transforms; TTF uses the standard counterclockwise angle sense).
  Auto-detected like the image codecs (`HAVE_FREETYPE`); when FreeType is not
  available the loader returns `IFunctionNotImplemented` and BDF fonts are
  unaffected.
- The `IBLEND_OVER` blend mode now applies to every primitive — Phase B of the
  alpha/anti-aliasing work. The core point-write (`_ISetPoint`) composites
  source-over when the GC blend mode is `IBLEND_OVER`, so lines, rectangles,
  polygons, circles, arcs, fills, flood fill, and text all honor it (previously
  only `IDrawPoint` did). The default `IBLEND_REPLACE` is unchanged.
- PNG alpha I/O — Phase D of the alpha/anti-aliasing work: the PNG reader now
  produces an RGBA image when the file has alpha (or a tRNS chunk; grey/palette
  inputs are normalized to RGB/RGBA), and the writer emits `PNG_COLOR_TYPE_RGBA`
  for alpha images. Formats that cannot store alpha still flatten over white.
- Anti-aliased drawing — Phase C of the alpha/anti-aliasing work:
  `ISetAntiAlias()` toggles per-GC anti-aliasing. When on, `IDrawLine()` renders
  smooth thin solid lines (Xiaolin Wu), `IDrawCircle()` uses a Wu circle
  rasterizer, `IFillPolygon()` fills with supersampled edge coverage (also
  smoothing filled rectangles/triangles/area shapes), `IFillCircle()` /
  `IFillEllipse()` fill smooth disks/ellipses, `IDrawEllipse()` draws a smooth
  outline (implicit-distance method), and `IDrawArc()`/`IFillArc()` render
  smooth partial arcs and pie wedges (angle-clipped coverage). Thick and dashed
  lines keep their aliased rasterizer.
- Optional alpha channel (RGBA) and source-over compositing — Phase A of the
  alpha/anti-aliasing work (see `docs/design/alpha-aa.md`):
  `ICreateImage(IOPTION_ALPHA)`, `IAllocColorAlpha()`, `IGetPixelAlpha()` /
  `ISetPixelAlpha()`, and a GC blend mode (`ISetBlendMode`, `IBLEND_OVER`)
  honored by `IDrawPoint()`. RGBA images are flattened over white when written
  to formats that have no alpha.
- `IGetPixel()` / `ISetPixel()`: public per-pixel read/write accessors (RGB,
  GC-independent), with bounds and handle validation.
- `IReduceColors()`: median-cut color quantization to fit an image into a
  bounded palette. The GIF writer now calls it automatically, replacing the
  previous first-256-colors hack (which collapsed all excess colors to one
  palette entry) so >256-color images produce a proper 8-bit GIF.
- BMP write support (`IFORMAT_BMP`): 24-bit uncompressed output; greyscale
  images are expanded to RGB. BMP was previously read-only.
- CMake build system with optional codec detection, install rules, an exported
  CMake package config (`find_package(Ilib)`), and a relocatable `ilib.pc`.
- GitHub Actions CI: gcc/clang on Linux and macOS, a no-optional-codecs build,
  AddressSanitizer/UBSan, coverage, a libFuzzer decoder smoke test, and a
  blocking `clang-tidy` static-analysis gate.
- Test suite (greatest + CTest) covering handle validation, drawing,
  format round-trips, and malformed-input decoder hardening.
- Doxygen API reference published to GitHub Pages; man pages for the client
  tools; a usage tutorial in `docs/`.
- Packaging: bundled BDF fonts install to `share/ilib/fonts`; CPack source and
  binary tarballs; a tag-triggered release workflow producing artifacts with
  checksums.
- `SPDX-License-Identifier` headers across the sources.

### Changed
- Converted all K&R function definitions to ISO C prototypes; the library and
  clients build warning-clean under `-Wall -Wextra` (`-Werror` in CI).
- GIF support ported to giflib 5.x.
- Single-sourced the version (`src/Ilib.h`) and de-duplicated the public header.
- Fixed the dead `ilib.sourceforge.net` URLs.

### Fixed
- Use-after-free in the BDF font glyph cache: `IFontBDFGetChar` cached the font
  by name pointer but `IFontBDFFree` never invalidated it, so a freed font whose
  name address was later reused could resolve to the dangling font. The cache is
  now cleared on free.
- Numerous decoder bugs found via fuzzing and sanitizers: heap overflows,
  out-of-bounds reads, integer-overflow allocations, and leaks on error paths
  in the PPM/PGM/XPM/BMP/GIF/PNG/JPEG readers; a GIF reader infinite loop on
  image-less input.
- BMP reader now skips the 4-byte row padding in the uncompressed paths; it
  previously misread any BMP whose row stride was not already 4-aligned.
- Null-pointer dereferences in the GC setters, font parser, and text drawing.
- A use-after-free in the font cache and several smaller correctness issues.

### Removed
- Legacy hand-written makefiles and a half-finished Autotools setup.
- The never-compiling `IXBM.c` and the bespoke `h2html.pl` doc generator.

## [1.1.10]

Baseline release prior to the modernization work described above.

[Unreleased]: https://github.com/craigk5n/ilib/compare/v1.1.10...HEAD
[1.1.10]: https://github.com/craigk5n/ilib/releases/tag/v1.1.10
