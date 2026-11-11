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
# (3D, virtual keyboard, PDF) and of database drivers (Mimer, ODBC, PostgreSQL)
# that the trim below removes; those messages are expected.
"$QT/bin/macdeployqt" "$APP" -qmldir="$DIR/src/app/qml" -libpath="$QT/lib" -no-codesign -verbose=0

# macdeployqt copies every QML module Qt might need, about 48 MB the app never
# loads. Tracing the packaged app through every screen and both report formats
# (DYLD_PRINT_LIBRARIES) showed these are never opened, so they're left out:
#   - the other Qt Quick Controls styles: the app draws with Controls.Basic and
#     its own theme (FluentWinUI3, Imagine, iOS, Material, Universal, Fusion,
#     macOS, Native);
#   - SQL and LocalStorage (no database), Particles, VectorImage, the Shapes and
#     Qt Design Studio helpers, XmlListModel, FolderListModel, the QML tooling;
#   - the TLS, network-status and SVG icon plugins (the app only talks plain TCP
#     to the adapter, and draws its icons itself).
# A named list, not a pattern, so a new Qt version can't quietly lose something
# the app needs. The checks below catch a mistake: a dangling link, or a screen
# that no longer opens.
C="$APP/Contents"
for fw in QtQuickControls2FluentWinUI3 QtQuickControls2FluentWinUI3StyleImpl \
          QtQuickControls2Imagine QtQuickControls2ImagineStyleImpl \
          QtQuickControls2IOS QtQuickControls2IOSStyleImpl \
          QtQuickControls2Material QtQuickControls2MaterialStyleImpl \
          QtQuickControls2Universal QtQuickControls2UniversalStyleImpl \
          QtQuickControls2Fusion QtQuickControls2FusionStyleImpl \
          QtQuickControls2Macos QtQuickControls2MacOSStyleImpl \
          QtQuickControls2Native QtQuickNativeStyle \
          QtQuickParticles QtQuickVectorImage QtQuickVectorImageGenerator QtQuickVectorImageHelpers \
          QtQuickShapesDesignHelpers QtQmlDesignSupport QtQmlXmlListModel QtLabsFolderListModel \
          QtQmlLocalStorage QtSql; do
    rm -rf "$C/Frameworks/$fw.framework"
done
rm -rf "$C/PlugIns/sqldrivers" "$C/PlugIns/tls" "$C/PlugIns/networkinformation" "$C/PlugIns/iconengines"
for style in fluentwinui3 imagine ios material universal fusion macos; do
    rm -f "$C/PlugIns/quick/libqtquickcontrols2${style}styleplugin.dylib" \
          "$C/PlugIns/quick/libqtquickcontrols2${style}styleimplplugin.dylib"
done
for plugin in qtquicknativestyle qtquickcontrols2nativestyle particles qquickvectorimage qquickvectorimagehelpers \
              qtquickshapesdesignhelpers qmldesignsupport qmlxmllistmodel qmlfolderlistmodel qmllocalstorage quicktooling; do
    rm -f "$C/PlugIns/quick/lib${plugin}plugin.dylib"
done
for dir in QtQuick/Controls/FluentWinUI3 QtQuick/Controls/Imagine QtQuick/Controls/iOS QtQuick/Controls/Material \
           QtQuick/Controls/Universal QtQuick/Controls/Fusion QtQuick/Controls/macOS QtQuick/Controls/Native \
           QtQuick/Controls/designer QtQuick/NativeStyle QtQuick/Particles QtQuick/VectorImage \
           QtQuick/Shapes/DesignHelpers QtQuick/LocalStorage QtQuick/tooling \
           QtQml/DesignSupport QtQml/XmlListModel Qt/labs/folderlistmodel; do
    rm -rf "$C/Resources/qml/$dir"
done

# Every Qt library a remaining binary links to must still be in the app.
echo "Checking nothing links to a library that was left out..."
MISSING=$(find "$C" -type f \( -perm -u+x -o -name '*.dylib' \) | while IFS= read -r f; do
    otool -L "$f" 2>/dev/null | awk 'NR > 1 { print $1 }' | grep '^@rpath/Qt' | while IFS= read -r dep; do
        lib=${dep#@rpath/}
        [ -e "$C/Frameworks/$lib" ] || echo "${f#$C/} needs ${lib%%/*}"
    done
done | sort -u)
if [ -n "$MISSING" ]; then
    echo "$MISSING" >&2
    exit 1
fi
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

# Open every screen from the packaged app, with its own libraries only: a module
# left out by mistake shows up as a screen that fails to load. The windows flash
# up for a moment each.
echo "Opening every screen of the packaged app..."
CHECK=$(mktemp -d)
for page in 0 1 2 3 4 5 6 7 8 9 10; do
    OBDARYN_SETTINGS_DIR="$CHECK/settings-$page" OBDARYN_RECORDINGS_DIR="$CHECK/rec" OBDARYN_REPORTS_DIR="$CHECK/rep" \
    OBDARYN_DEMO=1 OBDARYN_PAGE=$page OBDARYN_CAPTURE="$CHECK/page-$page.png" OBDARYN_DELAY_MS=2500 \
        "$C/MacOS/Obdaryn" > "$CHECK/page-$page.log" 2>&1 || true
    if [ ! -s "$CHECK/page-$page.png" ] || grep -qE "is not installed|Cannot load library|Library not loaded|failed to load component|Type .* unavailable" "$CHECK/page-$page.log"; then
        echo "Screen $page didn't open from the packaged app:" >&2
        grep -E "QML|qml|Library|library|module|plugin" "$CHECK/page-$page.log" | head -20 >&2
        exit 1
    fi
done
rm -rf "$CHECK"

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
