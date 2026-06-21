# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims
to follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

A modernization effort brought the late-1990s codebase up to current practice
without breaking the public API.

### Added
- Charting layer (`Ichart.h` / `IChart.c`), built on the public drawing API: an
  `IChart` object with title, axis labels, categories, an optional font, and one
  or more data series, rendered to a new image by `IChartRender()`. Three chart
  types — line (multi-series with markers), grouped bar, and pie — with axes,
  gridlines, numeric tick labels, category labels, a legend, and a default
  color palette. Exposed in the Python bindings (`ilib.Chart`) and demoed by the
  new `ilib-chart` example (which also showcases `IMontage`).
- Composition operations (`ICompose.c`): `ITrim()` (autocrop a uniform border,
  matched to the top-left pixel within a tolerance), `IBorder()` (add a solid
  frame), `IAppend()` (combine images side-by-side or stacked), and `IMontage()`
  (grid / contact-sheet layout with spacing and centered cells). `IAppend` /
  `IMontage` return a new image. Exposed in the Python bindings
  (`Image.trim/border` and `Image.append/montage` classmethods); `--trim` and
  `--border` added to both CLIs (border uses the `--background` color).
- More image filters (point operations) in `IFilter.c`: `INormalize()`
  (auto-stretch contrast), `ISepia()` (warm sepia tone, colour images only),
  and `IOpacity()` (scale an RGBA image's alpha by a factor). Exposed in the
  Python bindings (`Image.normalize/sepia/opacity`).
- AVIF read/write support (`IAVIF.c`, format `IFORMAT_AVIF`, extension
  `.avif`) via libavif, auto-detected with pkg-config (`HAVE_AVIFLIB`; toggle
  with `ILIB_WITH_AVIF`). Decoding yields RGB or RGBA (high-bit-depth AVIFs are
  down-converted to 8-bit); encoding takes RGB/RGBA (greyscale expanded) at a
  fixed quality. New error codes `IAVIFError` / `INoAVIFSupport`. Exposed in the
  Python bindings (`Format.AVIF`) and both CLIs.
- WebP read/write support (`IWEBP.c`, format `IFORMAT_WEBP`, extension
  `.webp`) via libwebp, auto-detected with pkg-config like the other optional
  codecs (`HAVE_WEBPLIB`; toggle with `ILIB_WITH_WEBP`). Decoding yields RGB, or
  RGBA when the file has alpha; encoding takes RGB/RGBA (greyscale is expanded)
  at a fixed high quality. New error codes `IWEBPError` / `INoWEBPSupport`.
  Exposed in the Python bindings (`Format.WEBP`) and both CLIs.
- New `ilib-mogrify` client tool: batch image editing. It applies the same
  operation pipeline as `ilib-convert` (in command-line order) to any number of
  files, overwriting each in place and keeping its format; with `--format EXT`
  it writes converted copies (new `<name>.EXT` files) instead, leaving the
  originals intact. Installed with a man page (`man ilib-mogrify`).
- The `ilib-convert` example tool gained an operation pipeline: it now applies
  any number of image operations (in command-line order) between read and
  write, exposing the filter/transform/convolution/resampling API as flags
  (`--greyscale`, `--negate`, `--brightness`, `--contrast`, `--gamma`,
  `--threshold`, `--flip`, `--flop`, `--rotate` + `--background`, `--blur`,
  `--gaussian-blur`, `--sharpen`, `--edge`, `--emboss`, `--resize`,
  `--reduce-colors`), with `--help`. The original `infile outfile` positional
  usage is unchanged.
- Resampling transforms (with interpolation), the fourth batch of
  ImageMagick-style transforms (`IResample.c`): `IResize()` (bilinear scaling to
  arbitrary dimensions, a higher-quality alternative to the nearest-neighbour
  `ICopyImageScaled`) and `IRotateAngle()` (rotation by an arbitrary angle, with
  the canvas grown to the rotated bounding box and revealed corners filled with
  a background color). Both interpolate all channels and are exposed in the
  Python bindings (`Image.resize`, `Image.rotate_angle`).
- Convolution / area filters, the third batch of ImageMagick-style transforms
  (`IConvolve.c`): a generic `IConvolve()` (arbitrary odd N x N kernel, with
  divisor and bias) plus `IBlur()` (box), `IGaussianBlur()`, `ISharpen()`,
  `IEdgeDetect()`, and `IEmboss()`. They sample neighbours with clamp-to-edge
  addressing, leave alpha untouched, and are exposed in the Python bindings
  (`Image.convolve/blur/gaussian_blur/sharpen/edge_detect/emboss`).
- Image transforms (geometric whole-image operations), the second batch of
  ImageMagick-style transforms (`ITransform.c`): `IFlip()` (vertical),
  `IFlop()` (horizontal), `IRotate()` (clockwise, by a multiple of 90 degrees;
  90/270 swap width and height), and `ICrop()`. They operate in place and are
  exposed in the Python bindings (`Image.flip/flop/rotate/crop`).
- Image filters (whole-image point operations), the first batch of
  ImageMagick-style transforms (`IFilter.c`): `IGreyscale()` (Rec.601 luma
  desaturation; `IGrayscale` alias), `INegate()`, `IBrightnessContrast()`
  (each control -100..100), `IGamma()`, and `IThreshold()`. They operate in
  place, leave alpha untouched, and are exposed in the Python bindings.
- Python bindings (`python/`) via cffi in ABI mode: load an installed
  `libilib` at runtime (no C compiler needed to install the package) and expose
  a Pythonic `Image` / `GC` / `Font` API covering the full drawing and image
  I/O surface (shapes, curves, anti-aliasing, RGBA/alpha, BDF + TrueType text,
  PPM/PGM/XPM/BMP/GIF/PNG/JPEG read/write). Includes a `unittest` suite and a
  CI job that builds the C library and runs the binding tests against it. The
  package is `pip`-installable (PyPI/git URL/local), single-sources its version,
  and ships a tag-triggered publish workflow using PyPI Trusted Publishing. The
  Python package versions independently; see [`python/CHANGELOG.md`](python/CHANGELOG.md).
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
- The bundled command-line tools and examples now install with an `ilib-`
  prefix (`iconvert` → `ilib-convert`, `ifont2h` → `ilib-font2h`, `iindex` →
  `ilib-index`, `idisplayfont` → `ilib-displayfont`, `ifraggraph` →
  `ilib-fraggraph`, `iwebreprt` → `ilib-webreprt`, `isample` → `ilib-sample`),
  so they are clearly namespaced in `$PATH`. Man pages were renamed to match
  (`man ilib-index`).
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
- The bundled Perl XS bindings (`perl/`). They were 1999-era alpha code that
  wrapped only a handful of functions, linked against the old `-lIlib`/`-lX11`
  build, and had a broken `IColor` typemap. Rather than carry incomplete
  bindings, they have been dropped; language bindings can be revisited later.
- Legacy hand-written makefiles and a half-finished Autotools setup.
- The never-compiling `IXBM.c` and the bespoke `h2html.pl` doc generator.

## [1.1.10]

Baseline release prior to the modernization work described above.

[Unreleased]: https://github.com/craigk5n/ilib/compare/v1.1.10...HEAD
[1.1.10]: https://github.com/craigk5n/ilib/releases/tag/v1.1.10
