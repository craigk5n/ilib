#!/bin/sh
# Generate sample output images for the bundled Ilib client tools.
#
# Usage:  docs/samples/generate.sh [build-dir]
#
# Run from the repository root after building (default build dir: ./build).
# Outputs PNGs into docs/images/clients/.
set -e

BUILD="${1:-build}"
CLIENTS="$BUILD/clients"
EXAMPLES="$BUILD/examples"
CONVERT="$EXAMPLES/ilib-convert"
SAMPLES="docs/samples"
OUT="docs/images/clients"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$OUT"

echo "ilib-displayfont -> glyph table"
# idisplayfont only enables -png/-gif when built with those codecs visible to
# the client; its default PPM output always works, so convert that to PNG.
"$CLIENTS/ilib-displayfont" fonts/helvB18.bdf > "$TMP/font.ppm"
"$CONVERT" "$TMP/font.ppm" "$OUT/displayfont.png"

echo "ilib-fraggraph -> frag efficiency graph"
"$CLIENTS/ilib-fraggraph" Ace "$SAMPLES/frags.log" > "$TMP/frags.gif"
"$CONVERT" "$TMP/frags.gif" "$OUT/fraggraph.png"

echo "ilib-webreprt -> web traffic by hour"
"$CLIENTS/ilib-webreprt" -tod -bar "$SAMPLES/access.log" > "$TMP/web.gif"
"$CONVERT" "$TMP/web.gif" "$OUT/webreprt.png"

echo "ilib-index -> thumbnail contact sheet"
# Build a few varied thumbnails (also a quick ilib-convert showcase).
"$CONVERT" docs/images/charts.png "$TMP/a.png" --resize 240x120
"$CONVERT" docs/images/charts.png "$TMP/b.png" --resize 240x120 --greyscale
"$CONVERT" docs/images/charts.png "$TMP/c.png" --resize 240x120 --sepia
"$CONVERT" docs/images/charts.png "$TMP/d.png" --resize 240x120 --negate
cp images/ilib-logo.png "$TMP/e.png"
"$CONVERT" docs/images/charts.png "$TMP/f.png" --resize 240x120 --edge
"$CLIENTS/ilib-index" -f "$OUT/index.png" \
  "$TMP/a.png" "$TMP/b.png" "$TMP/c.png" "$TMP/d.png" "$TMP/e.png" "$TMP/f.png"

echo "Wrote images to $OUT/"
