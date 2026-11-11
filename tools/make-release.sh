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
# Without an Apple Developer ID the app isn't notarized: the first time,
# open it with right-click → Open (see docs/RELEASE.md).
set -e

DIR=$(cd "$(dirname "$0")/.." && pwd)
QT=${QT:-/opt/homebrew/opt/qt}
BUILD="$DIR/build-release"
DIST="$DIR/dist"
VERSION=$(sed -n 's/^ *VERSION \([0-9.]*\).*/\1/p' "$DIR/CMakeLists.txt" | head -1)

echo "Obdaryn $VERSION: building (Release)..."
cmake -S "$DIR" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QT" > /dev/null
cmake --build "$BUILD"
ctest --test-dir "$BUILD" --output-on-failure

echo "Making the app self-contained..."
rm -rf "$DIST" && mkdir -p "$DIST"
cp -R "$BUILD/src/app/Obdaryn.app" "$DIST/"
APP="$DIST/Obdaryn.app"
# macdeployqt reports unresolved libraries of QML plugins Obdaryn never loads
# (3D, virtual keyboard, PDF); those messages are expected.
"$QT/bin/macdeployqt" "$APP" -qmldir="$DIR/src/app/qml" -libpath="$QT/lib" -no-codesign -verbose=0
# The build's Homebrew search path would hide a missing library on this Mac.
install_name_tool -delete_rpath "$QT/lib" "$APP/Contents/MacOS/Obdaryn" 2>/dev/null || true

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
