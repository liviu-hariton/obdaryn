#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Publishes the installer built by tools/make-release.sh as a GitHub release, tagged
# v<version>, so the website's download button (which links to
# releases/download/v<version>/Obdaryn-<version>.dmg) works. Needs the GitHub CLI,
# signed in once: brew install gh && gh auth login
#
# Usage: tools/publish-release.sh            (after tools/make-release.sh)
# Then deploy the site, so it names the new version: tools/deploy-site.sh
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9.]+).*/\1/p' "$DIR/CMakeLists.txt" | head -1)"
DMG="$DIR/dist/Obdaryn-$VERSION.dmg"
TAG="v$VERSION"

command -v gh >/dev/null || { echo "Install the GitHub CLI: brew install gh && gh auth login" >&2; exit 1; }
[[ -f "$DMG" ]] || { echo "No $DMG: run tools/make-release.sh first." >&2; exit 1; }
[[ -z "$(git -C "$DIR" status --porcelain --untracked-files=no)" ]] || { echo "Commit your changes first." >&2; exit 1; }

SHA="$(shasum -a 256 "$DMG" | cut -d' ' -f1)"
NOTES="Obdaryn $VERSION for macOS 14.4 (Sonoma) or later, on Apple Silicon and Intel.

The app isn't notarized yet: the first time, open it, then go to System Settings → Privacy & Security and click Open Anyway. Details: https://obdaryn.eu/download/

SHA-256 of Obdaryn-$VERSION.dmg:
\`$SHA\`"

echo "Publishing $TAG with $(basename "$DMG") (sha256 $SHA)"
read -r -p "Continue? [y/N] " answer
[[ "$answer" == "y" || "$answer" == "Y" ]] || { echo "Cancelled."; exit 0; }

# The tag points at the commit the installer was built from.
git -C "$DIR" tag -a "$TAG" -m "Obdaryn $VERSION" 2>/dev/null || echo "Tag $TAG already exists; using it."
git -C "$DIR" push origin "$TAG"
gh release create "$TAG" "$DMG" --repo liviu-hariton/obdaryn --title "Obdaryn $VERSION" --notes "$NOTES" --latest
echo "Published: https://github.com/liviu-hariton/obdaryn/releases/tag/$TAG"
