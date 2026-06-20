# Design proposal: alpha channel + anti-aliasing

Status: **Draft / RFC** · Target: IDEAS.md Phase 10 items #1 (anti-aliasing)
and #2 (alpha/RGBA + blending) · Author: modernization effort, 2026-06-20

## 1. Motivation

Ilib today renders with hard (aliased) edges and has no per-pixel transparency
— only a single GIF-style "transparent color" key (`IImageP.transparent`). For
charting/graphing output (the Phase 10 goal) the two biggest quality gaps are
**anti-aliasing** (smooth lines, curves, text) and an **alpha channel with
compositing** (translucent fills, overlaid markers, and the blending that AA
itself requires). The two are coupled: anti-aliasing is implemented *by*
alpha-blending edge coverage, so the alpha/compositing core comes first.

This document proposes how RGBA can coexist with the current RGB/greyscale
model **without breaking the public API**, and how AA is layered on top.

## 2. Current state (what we must not break)

- `IImageP` (internal, `IlibP.h`): `width`, `height`, `data`, `greyscale`,
  `interlaced`, `transparent` (an `IColorP *` color key), `comments`.
- Pixel storage in `data`: **1 byte/pixel** when `greyscale`, otherwise
  **3 bytes/pixel** RGB. Offsets are computed inline in the
  `_ISetPoint` / `_ISetPointRGB` / `_IGetPointColor` macros (`IlibP.h`) and in a
  few decoders.
- `IColorP`: `red`/`green`/`blue` (+ `value`); **no alpha**.
- `IGCP`: has an `antialiased` flag, but it is **font-only** today (renders the
  glyph at 2× and halves — see `ISetAntiAliasedFont`); primitives ignore it.
- Formats: the PNG codec **strips** alpha on read (`png_set_strip_alpha`) and
  writes `PNG_COLOR_TYPE_RGB`; no format round-trips alpha.
- Guiding constraints (IDEAS.md): preserve the public API, keep optional codecs
  optional, change internals incrementally and reversibly.

## 3. Goals / non-goals

**Goals**
- An optional 4th (alpha) channel per image, opt-in at creation.
- Straight (non-premultiplied) 8-bit alpha, source-over compositing.
- Alpha-aware colors and a GC blend mode; existing opaque drawing unchanged.
- Anti-aliased line / polyline / rectangle / circle / ellipse / arc / polygon,
  toggled per-GC, plus AA text via FreeType (Phase 9) later.
- PNG reads/writes RGBA; other formats degrade predictably.

**Non-goals (initially)**
- Greyscale+alpha (2-channel) images — defer; alpha implies color (4ch).
- Premultiplied-alpha pipeline, gamma-correct blending, sub-pixel/LCD AA.
- Vector output (SVG). Arbitrary affine transforms.

## 4. Image model

Add an explicit channel count rather than overloading flags.

```c
/* IlibP.h — IImageP gains: */
unsigned short channels;   /* 1 = greyscale, 3 = RGB, 4 = RGBA */
unsigned short has_alpha;  /* convenience: channels == 4 */
```

`greyscale` stays (channels == 1). Storage stride becomes
`(y * width + x) * channels`. RGBA pixels are `[R,G,B,A]`, A = 255 opaque.

**Creation.** A new option bit:

```c
#define IOPTION_ALPHA 0x0002   /* RGBA image (ICreateImage) */
```

`ICreateImage(w, h, IOPTION_ALPHA)` allocates `w*h*4`, initialized to opaque
white (matching the current "blank = white" behavior, A = 255). `IOPTION_ALPHA`
with `IOPTION_GREYSCALE` is rejected (`IInvalidArgument`) for now.

**Pixel access macros.** `_ISetPoint`/`_IGetPointColor`/`_ISetPointRGB` switch
on `channels`. To keep the common cases fast, keep the existing 1- and 3-channel
branches and add a 4-channel branch; the offset multiply uses the literal
stride per branch (no general multiply in the hot path). The blend path (below)
is only taken when compositing is requested.

## 5. Color + GC

Add alpha to `IColorP` (default 255 keeps every existing color opaque):

```c
unsigned char alpha;   /* 0 = transparent .. 255 = opaque */
```

Public additions (existing functions unchanged; `IAllocColor` sets alpha=255):

```c
IColor IAllocColorAlpha(unsigned r, unsigned g, unsigned b, unsigned a);
IError ISetForegroundAlpha(IGC gc, IColor color);   /* color may carry alpha */
```

Blend mode on the GC controls how the foreground is written:

```c
typedef enum {
  IBLEND_REPLACE = 0,  /* overwrite (current behavior; the default) */
  IBLEND_OVER          /* Porter-Duff source-over using the source alpha */
} IBlendMode;
IError ISetBlendMode(IGC gc, IBlendMode mode);
```

`IBLEND_REPLACE` as the default is what preserves today's output exactly. AA
drawing (Section 6) implies `OVER` regardless of the mode, because it must
blend fractional coverage.

**Source-over (straight alpha), per channel:**

```
a  = src_a / 255
out_rgb = src_rgb * a + dst_rgb * (1 - a)
out_a   = src_a + dst_a * (1 - a)        /* only when dst has alpha */
```

For a 3-channel destination there is no `dst_a`; we composite the source over
the existing opaque RGB and store RGB (the destination stays opaque). This lets
translucent/AA drawing work on ordinary RGB images (the common chart case)
without requiring an RGBA buffer.

## 6. Anti-aliasing

AA is **coverage → alpha**: each primitive computes a per-pixel coverage in
`[0,1]` at edges and composites `foreground` with `alpha = coverage * src_a`
using source-over. Interiors have coverage 1.

Enabled per-GC, reusing/renaming the existing flag concept:

```c
IError ISetAntiAlias(IGC gc, int on);   /* primitives render AA when on */
```

When `on`, `IDrawLine`, `IDrawRectangle`, `IDrawPolygon`, `IDrawCircle`,
`IDrawEllipse`, `IDrawArc`, and the `IFill*` counterparts use AA rasterizers;
when off they use the current integer rasterizers (zero behavior change). The
font `antialiased` flag is kept distinct (or folded in) — its current 2×
hack would be superseded by FreeType AA in the Phase 9 font work.

**Algorithms (incremental):**
- **Lines:** Xiaolin Wu's algorithm (fast, classic) for thin lines; thick AA
  lines as an AA polygon (the line's quad) once joins/caps land.
- **Circles/ellipses/arcs:** distance-to-edge coverage (1 px feather) around the
  current Bresenham/Midpoint path.
- **Polygons / filled shapes:** scanline rasterization with per-edge fractional
  coverage (analytic) — or, as a simpler first cut, 4× supersampled coverage,
  then optimize.

A shared internal helper does the compositing so every primitive is consistent:

```c
/* coverage in [0,255]; composites gcp->foreground over (x,y). */
void _IBlendPoint(IImageP *img, IGCP *gc, int x, int y, unsigned cover);
```

## 7. Format I/O

- **PNG:** read RGBA when the file has alpha (stop calling
  `png_set_strip_alpha`; set `channels=4`); write `PNG_COLOR_TYPE_RGBA` when
  `has_alpha`. This is the only format that round-trips alpha and is the
  natural primary target.
- **GIF:** no true alpha. On write, threshold (`A < 128` → the transparent
  index) so existing index-transparency keeps working.
- **JPEG / BMP / PPM / PGM / XPM:** no alpha. On write, **flatten**: composite
  RGBA over an opaque background (default white; optionally a settable
  background) and emit RGB. On read these stay 3-channel/`has_alpha=0`.
- `IFileType`/dispatch unchanged; the per-format writers gain a flatten step
  guarded by `image->has_alpha`.

## 8. Public pixel accessors

Extend the just-added accessors rather than changing them:

```c
IError ISetPixelAlpha(IImage, int x, int y, unsigned r, unsigned g,
                      unsigned b, unsigned a);
IError IGetPixelAlpha(IImage, int x, int y, unsigned *r, unsigned *g,
                      unsigned *b, unsigned *a);
```

`ISetPixel`/`IGetPixel` keep working on RGBA images (alpha defaults to 255 on
set; ignored on get).

## 9. Backward compatibility

- No existing signature changes; new symbols only. `channels`/`has_alpha`/
  `alpha` live behind the opaque handles.
- RGB and greyscale images are byte-for-byte identical to today; default blend
  mode is `REPLACE` and AA is off, so unmodified callers see no change.
- `IColorP` grows one byte — internal struct, not ABI-exposed to clients
  (handles are opaque), but it does bump the library `SOVERSION` (additive +
  struct growth → minor version; keep `SOVERSION` = major per policy).
- Tests: the existing suite must pass unchanged; add RGBA round-trip
  (create → set alpha → PNG write/read → verify), blend math unit tests, and
  golden-pixel AA tests (coverage at a known edge).

## 10. Phasing (each phase ships independently, green)

- **A — model + compositing core:** `channels`/`has_alpha`, `IOPTION_ALPHA`,
  macro stride update, `IColorP.alpha` + `IAllocColorAlpha`,
  `IGet/ISetPixelAlpha`, `_IBlendPoint`, `ISetBlendMode`. Unlocks translucent
  pixel work; no AA yet.
- **B — alpha-aware fills/draws:** `ISetForegroundAlpha`; `IBLEND_OVER` honored
  by the existing primitives (translucent solid fills/lines).
- **C — anti-aliasing:** `ISetAntiAlias` + AA line/circle/ellipse/arc/polygon
  via `_IBlendPoint`.
- **D — format alpha:** PNG RGBA read/write; flatten path for the rest; GIF
  alpha threshold.
- **E (later, Phase 9 overlap):** FreeType AA text; curves (Bézier/spline)
  which compose naturally with the AA rasterizer.

## 11. Risks & open questions

- **Macro reach.** The pixel macros are used across many `.c` files; the
  stride change is the riskiest edit. Mitigation: keep per-channel literal-stride
  branches, add tests first, land Phase A behind `IOPTION_ALPHA` so RGB paths
  are untouched.
- **Hot-path cost.** Blending adds work to the inner draw loop. Mitigation:
  `REPLACE` stays a plain store; only `OVER`/AA pay for the blend.
- **Straight vs premultiplied alpha.** Proposed: straight, to match PNG and keep
  the API obvious. Revisit if performance or repeated-composite error matters.
- **`transparent` vs alpha.** Both can be set. Proposal: alpha is authoritative
  for on-image compositing; `transparent` remains the GIF export key. Document
  the precedence; consider deriving the GIF key from alpha when only alpha is
  set.
- **Greyscale+alpha** and **16-bit channels** are deferred; confirm that's
  acceptable for the charting use case (it is for 8-bit web output).
- **AA fill correctness** at shared polygon edges (double-blending seams) needs
  analytic coverage or a coverage buffer; the 4× supersample first cut avoids
  seams at the cost of speed.

## 12. Recommendation

Land **Phase A** first (model + compositing core, fully behind `IOPTION_ALPHA`)
as a self-contained PR with thorough tests, then B–D in sequence. Phase A is
the foundation everything else builds on and carries the only structural risk;
proving it green keeps the rest low-risk and incremental.
