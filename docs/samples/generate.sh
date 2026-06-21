#!/bin/sh
# Generate sample output images for the bundled Ilib client tools.
#
# Usage:  docs/samples/generate.sh [build-dir]
#
# Run from the repository root after building (default build dir: ./build).
# Outputs PNGs into docs/images/clients/. Needs libpng (the tools now write
# PNG directly via their -png option).
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
"$CLIENTS/ilib-displayfont" -png fonts/helvB18.bdf > "$OUT/displayfont.png"

echo "ilib-fraggraph -> frag efficiency graph"
"$CLIENTS/ilib-fraggraph" -png Ace "$SAMPLES/frags.log" > "$OUT/fraggraph.png"

echo "ilib-webreprt -> web traffic by hour"
"$CLIENTS/ilib-webreprt" -tod -bar -png "$SAMPLES/access.log" > "$OUT/webreprt.png"

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
