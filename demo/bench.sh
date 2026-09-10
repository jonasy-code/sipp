#!/bin/sh
#
# bench.sh - render every SIPP demo at a fixed resolution and report
#            wall-clock seconds per demo, plus a total.
#
# Usage:  ./bench.sh [size]        (default size: 256)
#
# Run it once before you start optimizing and keep the output; then
# re-run after each change.  Times are wall-clock via the shell's
# builtin "time" (portable across macOS and Linux; no GNU time needed).
#
# Notes:
#   * All demos use PHONG shading and 2x2 oversampling by default, so a
#     256x256 request actually rasterizes 512x512 sub-pixels.
#   * "scroll" reads sipp.bm from the current directory.
#   * Output images are written to the current directory (*.ppm).

SIZE=${1:-256}
DEMOS="torustest conetest ellipsoid prismtest chain scroll teapot structure planettest isy90 strausstest woodtest"

cd "$(dirname "$0")" || exit 1

echo "SIPP benchmark, ${SIZE}x${SIZE}, PHONG, oversampling 2"
echo "host: $(uname -srm)"
echo "-----------------------------------------------"

TOTAL_START=$(date +%s)
for d in $DEMOS; do
    if [ ! -x "./$d" ]; then
        printf "%-12s (not built)\n" "$d"
        continue
    fi
    START=$(date +%s.%N 2>/dev/null || date +%s)
    ./"$d" -s "$SIZE" -j 0 -a >/dev/null 2>&1
    STATUS=$?
    END=$(date +%s.%N 2>/dev/null || date +%s)
    ELAPSED=$(echo "$END - $START" | bc 2>/dev/null || echo "?")
    if [ $STATUS -eq 0 ]; then
        printf "%-12s %8.2f s\n" "$d" "$ELAPSED"
    else
        printf "%-12s FAILED (exit %d)\n" "$d" "$STATUS"
    fi
done
TOTAL_END=$(date +%s)
echo "-----------------------------------------------"
echo "total: $((TOTAL_END - TOTAL_START)) s"
