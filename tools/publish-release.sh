#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Publishes the installer built by tools/make-release.sh as a GitHub release, tagged
# v<version>, so the website's download button (which links to
# releases/download/v<version>/Obdaryn-<version>.dmg) works. Needs the GitHub CLI,
# signed in once: brew install gh && gh auth login
#
# What changed goes in docs/releases/<version>.md, written and committed before
# publishing: Markdown, starting with what's new and what's fixed. The script adds
# the opening line (version and macOS), the first-launch note and the checksum
# around it. Without that file the release gets no changelog (it asks first).
#
# Usage: tools/publish-release.sh            (after tools/make-release.sh)
#        tools/publish-release.sh --dry-run  (prints the release notes, publishes nothing)
# Then deploy the site, so it names the new version: tools/deploy-site.sh
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9.]+).*/\1/p' "$DIR/CMakeLists.txt" | head -1)"
DMG="$DIR/dist/Obdaryn-$VERSION.dmg"
TAG="v$VERSION"
CHANGES_FILE="$DIR/docs/releases/$VERSION.md"
DRY_RUN=0
[[ "${1:-}" == "--dry-run" ]] && DRY_RUN=1

command -v gh >/dev/null || { echo "Install the GitHub CLI: brew install gh && gh auth login" >&2; exit 1; }
[[ -f "$DMG" ]] || { echo "No $DMG: run tools/make-release.sh first." >&2; exit 1; }
if (( ! DRY_RUN )); then
  [[ -z "$(git -C "$DIR" status --porcelain --untracked-files=no)" ]] || { echo "Commit your changes first." >&2; exit 1; }
  # The notes are part of the tagged commit, so they must be committed too.
  if [[ -f "$CHANGES_FILE" ]] && ! git -C "$DIR" ls-files --error-unmatch "$CHANGES_FILE" >/dev/null 2>&1; then
    echo "Commit docs/releases/$VERSION.md first." >&2
    exit 1
  fi
fi

SHA="$(shasum -a 256 "$DMG" | cut -d' ' -f1)"
INTRO="Obdaryn $VERSION for macOS 14.4 (Sonoma) or later, on Apple Silicon and Intel."
FOOTER="The app isn't notarized yet: the first time, open it, then go to System Settings → Privacy & Security and click Open Anyway. Details: https://obdaryn.eu/download/

SHA-256 of Obdaryn-$VERSION.dmg:
\`$SHA\`"
if [[ -f "$CHANGES_FILE" ]]; then
  NOTES="$INTRO

$(cat "$CHANGES_FILE")

$FOOTER"
else
  NOTES="$INTRO

$FOOTER"
fi

if (( DRY_RUN )); then
  printf '%s\n' "$NOTES"
  exit 0
fi

if [[ ! -f "$CHANGES_FILE" ]]; then
  echo "No docs/releases/$VERSION.md: this release will have no changelog."
  read -r -p "Publish without one? [y/N] " answer
  [[ "$answer" == "y" || "$answer" == "Y" ]] || { echo "Cancelled. Write docs/releases/$VERSION.md, commit it, and run again."; exit 0; }
fi

echo "Publishing $TAG with $(basename "$DMG") (sha256 $SHA)"
read -r -p "Continue? [y/N] " answer
[[ "$answer" == "y" || "$answer" == "Y" ]] || { echo "Cancelled."; exit 0; }

# The tag points at the commit the installer was built from.
git -C "$DIR" tag -a "$TAG" -m "Obdaryn $VERSION" 2>/dev/null || echo "Tag $TAG already exists; using it."
git -C "$DIR" push origin "$TAG"
gh release create "$TAG" "$DMG" --repo liviu-hariton/obdaryn --title "Obdaryn $VERSION" --notes "$NOTES" --latest
echo "Published: https://github.com/liviu-hariton/obdaryn/releases/tag/$TAG"
