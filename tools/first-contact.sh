#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
#
# First contact with a real car. Run it in the car, laptop joined to the
# adapter's Wi-Fi ("V-LINK"). It only reads; nothing is cleared or changed.
#
#   tools/first-contact.sh            # Vgate iCar Pro Wi-Fi at 192.168.0.10:35000
#   tools/first-contact.sh HOST PORT
#
# Writes logs/first-contact-<time>-<phase>.log, a recording of the same
# session (.obdrec) that the app can replay without the car ("Replay a
# recording" > "Open a file..."), and the shareable report as a web page
# (.html, the same one the app saves). All of them contain your VIN, so they
# are git-ignored; share them only if you want to.

HOST=${1:-192.168.0.10}
PORT=${2:-35000}
DIR=$(cd "$(dirname "$0")/.." && pwd)
PROBE="$DIR/build/src/cli/obdaryn-probe"
STAMP=$(date +%Y%m%d-%H%M)
mkdir -p "$DIR/logs"

if [ ! -x "$PROBE" ]; then
    echo "Build first: cmake --build build"; exit 1
fi

echo "Checking the adapter at $HOST:$PORT ..."
if ! nc -z -G 3 "$HOST" "$PORT" 2>/dev/null; then
    echo "Can't reach the adapter. Is this Mac on the adapter's Wi-Fi (V-LINK)?"
    echo "On macOS 15+, also allow your terminal app under System Settings > Privacy & Security > Local Network."
    exit 1
fi

run() {
    LOG="$DIR/logs/first-contact-$STAMP-$1.log"
    echo "Running the full report ($1). This takes about a minute ..."
    REC="$DIR/logs/first-contact-$STAMP-$1.obdrec"
    HTML="$DIR/logs/first-contact-$STAMP-$1.html"
    "$PROBE" "$HOST" "$PORT" --report --verbose --record "$REC" --report-html "$HTML" > "$LOG" 2>&1
    echo "  saved $LOG"
    echo "  the recording $REC"
    [ -s "$HTML" ] && echo "  and the report $HTML (open it in a browser)"
    grep -E "^  (VIN|Protocol|Adapter):|MIL |values/s|Module list|answered|not found|error" "$LOG" | grep -v "^    " | head -30
}

echo
echo "Step 1 of 2: ignition ON, engine OFF. Press Enter when ready."
read _
run ignition-on

echo
echo "Step 2 of 2: start the engine and let it idle. Press Enter when it runs (or Ctrl+C to stop here)."
read _
run engine-running

echo
echo "Done. Rejoin your normal Wi-Fi and tell Claude the logs are in logs/."
