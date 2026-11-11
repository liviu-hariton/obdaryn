#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Captures the app screenshots the website (site/) shows, from the built-in demo
# car, in English and Romanian, light and dark, at 2x. Run after building the app:
#
#   cmake --build build && tools/capture-site-images.sh [screen ...]
#
# Name screens (codes, modules, readiness, charts) to capture only those.
# Writes site/src/assets/screens/<screen>-<lang>-<theme>.png. Captures run one at a
# time: launching them back to back in parallel fails to create the offscreen buffer.
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
APP="$DIR/build/src/app/Obdaryn.app/Contents/MacOS/Obdaryn"
OUT="$DIR/site/src/assets/screens"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT"

[[ -x "$APP" ]] || { echo "Build the app first: cmake --build build" >&2; exit 1; }

# screen name | page index | crop (x,y,w,h in capture pixels, 2x) | extra environment
# The crops keep the part of each screen that proves what the site says about it,
# at a size where the app's text stays readable on the page. The site never shows a
# screenshot larger than the app's real size, so the two full-width screens come
# from a 1600-point window: cropped, they are at least the page's 1312-point
# column wide and fill it, lining up with the text above them. Charts caps its own
# content width, so its crop is exactly 1312 points, centred on the charts.
SCREENS=(
  "codes|1|1360,180,1240,1480|"
  "modules|8|480,340,2720,1260|OBDARYN_SIZE=1600x860 OBDARYN_SCAN_MODULES=1 OBDARYN_DELAY_MS=9000"
  "readiness|2|600,260,1900,1440|OBDARYN_READ_TESTS=1 OBDARYN_DELAY_MS=6000"
  "charts|10|520,208,2624,1092|OBDARYN_SIZE=1600x860 OBDARYN_DELAY_MS=135000"
)

for lang in en ro; do
  for theme in dark light; do
    for entry in "${SCREENS[@]}"; do
      IFS='|' read -r name page crop extra <<<"$entry"
      if (( $# > 0 )) && [[ " $* " != *" $name "* ]]; then continue; fi
      file="$OUT/$name-$lang-$theme.png"
      echo "capturing $(basename "$file")"
      # The offscreen platform has no screen-size limit, so the window (1320 x 860
      # unless the screen's extra environment sets OBDARYN_SIZE) renders whole at
      # scale 2. Each run gets its own settings and folders.
      env QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 \
          OBDARYN_SETTINGS_DIR="$TMP/settings-$name-$lang-$theme" \
          OBDARYN_RECORDINGS_DIR="$TMP/rec" OBDARYN_REPORTS_DIR="$TMP/rep" \
          OBDARYN_CAPTURE="$file" OBDARYN_PAGE="$page" OBDARYN_DEMO=1 \
          OBDARYN_APPEARANCE="$theme" OBDARYN_LANG="$lang" OBDARYN_SIZE=1320x860 \
          $extra "$APP" >/dev/null 2>&1
      [[ -s "$file" ]] || { echo "  failed: $file" >&2; exit 1; }
      IFS=',' read -r cx cy cw ch <<<"$crop"
      sips -c "$ch" "$cw" --cropOffset "$cy" "$cx" "$file" >/dev/null
    done
  done
done
echo "done: $OUT"
