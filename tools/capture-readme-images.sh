#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Captures the README's screenshots (docs/images/) from the built-in demo car, in
# English, at the app's real size (1x). Run after building the app:
#
#   cmake --build build && tools/capture-readme-images.sh
#
# It renders offscreen (no windows), keeps every run's settings in a throwaway
# folder, and makes what some screens need first: two short recordings for the
# comparison chart and a few saved trips for the trip history. It takes about
# ten minutes, most of it letting the charts and trips fill up in real time.
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP="$DIR/build/src/app/Obdaryn.app/Contents/MacOS/Obdaryn"
OUT="$DIR/docs/images"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
[[ -x "$APP" ]] || { echo "Build the app first: cmake --build build" >&2; exit 1; }

# run <settings-name> VAR=value ...: one offscreen run of the app with its own
# settings, recordings and reports folders. Captures run one at a time.
run() {
  local name="$1"; shift
  env QT_QPA_PLATFORM=offscreen OBDARYN_LANG=en \
      OBDARYN_SETTINGS_DIR="$TMP/settings-$name" OBDARYN_RECORDINGS_DIR="$TMP/rec" OBDARYN_REPORTS_DIR="$TMP/rep" \
      "$@" "$APP" >/dev/null 2>&1
}
shot() { echo "capturing $1"; }

# Two recordings of the demo car for "two recordings compared": the second starts
# 4 s later and is shorter, so the dashed line shows where the drives differ.
echo "recording two demo drives..."
mkdir -p "$TMP/rec"
run rec-1 OBDARYN_DEMO=1 OBDARYN_RECORD=1 OBDARYN_CAPTURE="$TMP/rec-1.png" OBDARYN_DELAY_MS=30000
run rec-2 OBDARYN_DEMO=1 OBDARYN_RECORD=1 OBDARYN_RECORD_AFTER_MS=4000 OBDARYN_CAPTURE="$TMP/rec-2.png" OBDARYN_DELAY_MS=16000
A="$(ls "$TMP/rec"/*.obdrec | head -1)"
B="$(ls "$TMP/rec"/*.obdrec | tail -1)"
[[ "$A" != "$B" ]] || { echo "  the two recordings weren't made" >&2; exit 1; }

# A few saved trips, so the Trip screen shows its history under the current trip.
# A trip is saved only from 0.5 km; the demo car covers about that in its first
# minute and then idles, so each trip runs two minutes or more.
echo "driving two demo trips..."
for i in 1 2; do
  run trips OBDARYN_DEMO=1 OBDARYN_DEMO_TRIPS=1 OBDARYN_CAPTURE="$TMP/trip-$i.png" OBDARYN_DELAY_MS=$((105000 + i * 15000))
done

shot overview-dark.png
run overview OBDARYN_DEMO=1 OBDARYN_PAGE=0 OBDARYN_APPEARANCE=dark OBDARYN_SIZE=1320x860 OBDARYN_DELAY_MS=12000 \
    OBDARYN_CAPTURE="$OUT/overview-dark.png"
shot trouble-codes-dark.png
run codes OBDARYN_DEMO=1 OBDARYN_PAGE=1 OBDARYN_APPEARANCE=dark OBDARYN_SIZE=1320x860 OBDARYN_DELAY_MS=8000 \
    OBDARYN_SELECT_CODE=P0420 OBDARYN_CAPTURE="$OUT/trouble-codes-dark.png"
shot all-modules-light.png
run modules OBDARYN_DEMO=1 OBDARYN_PAGE=8 OBDARYN_SCAN_MODULES=1 OBDARYN_APPEARANCE=light OBDARYN_SIZE=1320x860 \
    OBDARYN_DELAY_MS=20000 OBDARYN_CAPTURE="$OUT/all-modules-light.png"
shot charts-light.png
run charts OBDARYN_DEMO=1 OBDARYN_PAGE=10 OBDARYN_CHART_PRESET="Fuel mixture" OBDARYN_APPEARANCE=light \
    OBDARYN_SIZE=1320x860 OBDARYN_DELAY_MS=125000 OBDARYN_CAPTURE="$OUT/charts-light.png"
shot charts-compare-dark.png
run compare OBDARYN_PAGE=10 OBDARYN_CHART_A="$A" OBDARYN_CHART_B="$B" OBDARYN_CHART_PRESET="Speed and throttle" \
    OBDARYN_APPEARANCE=dark OBDARYN_SIZE=1320x860 OBDARYN_DELAY_MS=4000 OBDARYN_CAPTURE="$OUT/charts-compare-dark.png"
shot self-tests-light.png
run tests OBDARYN_DEMO=1 OBDARYN_PAGE=2 OBDARYN_READ_TESTS=1 OBDARYN_APPEARANCE=light OBDARYN_SIZE=1320x1300 \
    OBDARYN_DELAY_MS=10000 OBDARYN_CAPTURE="$OUT/self-tests-light.png"
shot trip-light.png
run trips OBDARYN_DEMO=1 OBDARYN_DEMO_TRIPS=1 OBDARYN_PAGE=9 OBDARYN_APPEARANCE=light OBDARYN_SIZE=1320x1000 \
    OBDARYN_DELAY_MS=30000 OBDARYN_CAPTURE="$OUT/trip-light.png"

for f in overview-dark trouble-codes-dark all-modules-light charts-light charts-compare-dark self-tests-light trip-light; do
  [[ -s "$OUT/$f.png" ]] || { echo "  failed: $f.png" >&2; exit 1; }
done
echo "done: $OUT"
