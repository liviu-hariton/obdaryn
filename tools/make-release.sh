#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds an installable Obdaryn for macOS: dist/Obdaryn-<version>.dmg
#
#   tools/make-release.sh
#
# Steps: Release build, unit tests, a self-contained Obdaryn.app (Qt copied
# in by macdeployqt), an ad-hoc signature, then a disk image with an
# Applications shortcut. Everything runs on this Mac; nothing is uploaded.
#
# The installer is built with the official Qt binaries, not Homebrew's: Homebrew
# builds Qt for the newest macOS only, the official Qt runs on macOS 14.4 and
# later, on Apple Silicon and Intel. Install it once (docs/RELEASE.md):
#   python3 -m venv /tmp/aqt && /tmp/aqt/bin/pip install aqtinstall
#   /tmp/aqt/bin/aqt install-qt mac desktop 6.12.0 clang_64 --outputdir ~/Qt
# Without an Apple Developer ID the app isn't notarized: the first time,
# open it with right-click → Open (see docs/RELEASE.md).
set -e

DIR=$(cd "$(dirname "$0")/.." && pwd)
QT=${QT:-$HOME/Qt/6.12.0/macos}
# The oldest macOS the official Qt supports, and both Mac architectures.
MIN_MACOS=14.4
ARCHS="arm64;x86_64"
BUILD="$DIR/build-release"
DIST="$DIR/dist"
VERSION=$(sed -n 's/^ *VERSION \([0-9.]*\).*/\1/p' "$DIR/CMakeLists.txt" | head -1)

if [ ! -x "$QT/bin/macdeployqt" ]; then
    echo "No official Qt in $QT: install it (see the top of this script), or set QT=..." >&2
    exit 1
fi

echo "Obdaryn $VERSION: building (Release, macOS $MIN_MACOS+, arm64 + x86_64)..."
# --fresh: a cached Qt location from an earlier build (Homebrew, say) must not carry over.
cmake --fresh -S "$DIR" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" -DCMAKE_OSX_ARCHITECTURES="$ARCHS" > /dev/null
cmake --build "$BUILD"
ctest --test-dir "$BUILD" --output-on-failure

echo "Making the app self-contained..."
rm -rf "$DIST" && mkdir -p "$DIST"
cp -R "$BUILD/src/app/Obdaryn.app" "$DIST/"
APP="$DIST/Obdaryn.app"
# macdeployqt reports unresolved libraries of QML plugins Obdaryn never loads
# (3D, virtual keyboard, PDF) and of the database drivers removed below; those
# messages are expected.
"$QT/bin/macdeployqt" "$APP" -qmldir="$DIR/src/app/qml" -libpath="$QT/lib" -no-codesign -verbose=0
# Qt's database drivers come along with Qt Quick's LocalStorage. Obdaryn uses no
# database; these three link to client libraries the app doesn't ship (Mimer,
# ODBC, PostgreSQL), so leave them out rather than ship broken plugins.
for driver in mimer odbc psql; do rm -f "$APP/Contents/PlugIns/sqldrivers/libqsql$driver.dylib"; done
# The build's Qt search path would hide a missing library on this Mac.
install_name_tool -delete_rpath "$QT/lib" "$APP/Contents/MacOS/Obdaryn" 2>/dev/null || true

# Every binary in the app must run on MIN_MACOS: a library built for a newer
# macOS (a stray Homebrew one, say) would stop the app from opening there.
echo "Checking every binary runs on macOS $MIN_MACOS..."
TOO_NEW=$(find "$APP" -type f \( -perm -u+x -o -name '*.dylib' \) | while IFS= read -r f; do
    vtool -show-build "$f" 2>/dev/null | awk -v f="$f" -v min="$MIN_MACOS" '
        /minos/ { split($2, a, "."); split(min, b, ".")
                  if (a[1] + 0 > b[1] + 0 || (a[1] + 0 == b[1] + 0 && a[2] + 0 > b[2] + 0)) print f " needs " $2 }'
done | sort -u)
if [ -n "$TOO_NEW" ]; then
    echo "These need a newer macOS than $MIN_MACOS:" >&2
    echo "$TOO_NEW" >&2
    exit 1
fi

echo "Signing (ad hoc)..."
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

echo "Making the disk image..."
STAGE="$DIST/stage"
mkdir -p "$STAGE"
mv "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
hdiutil create -volname "Obdaryn $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DIST/Obdaryn-$VERSION.dmg" > /dev/null
mv "$STAGE/Obdaryn.app" "$DIST/"
rm -rf "$STAGE"

# The About window computes its copyright years from the build year; the
# README's line is plain text, so remind when it falls behind.
LAST=$(grep -o "Copyright © [0-9–-]*" "$DIR/README.md" | grep -o "[0-9]\{4\}$")
if [ -n "$LAST" ] && [ "$LAST" -lt "$(date +%Y)" ]; then
    echo "Note: README.md says Copyright © ...$LAST; make it end in $(date +%Y)."
fi
echo "Done: $DIST/Obdaryn-$VERSION.dmg ($(du -h "$DIST/Obdaryn-$VERSION.dmg" | cut -f1))"
