#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the website (site/) and copies it to obdaryn.eu over SSH. The server is a
# HestiaCP account, which serves the domain from ~/web/obdaryn.eu/public_html.
#
# One-time setup: create site/.deploy.env (git-ignored) with
#   OBDARYN_SITE_SSH="user@server"          # an SSH login with a key, no password
#   OBDARYN_SITE_PORT="22"                  # optional
#   OBDARYN_SITE_PATH="web/obdaryn.eu/public_html/"   # optional, relative to the home folder
#
# Usage: tools/deploy-site.sh [--dry-run]
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
SITE="$DIR/site"
ENV_FILE="$SITE/.deploy.env"
DRY=""
[[ "${1:-}" == "--dry-run" ]] && DRY="--dry-run"

[[ -f "$ENV_FILE" ]] || { echo "Create $ENV_FILE first (see the top of this script)." >&2; exit 1; }
# shellcheck source=/dev/null
source "$ENV_FILE"
: "${OBDARYN_SITE_SSH:?set OBDARYN_SITE_SSH in site/.deploy.env}"
PORT="${OBDARYN_SITE_PORT:-22}"
REMOTE_PATH="${OBDARYN_SITE_PATH:-web/obdaryn.eu/public_html/}"

cd "$SITE"
[[ -d node_modules ]] || npm ci
npm run build

echo
echo "Upload site/dist/ to $OBDARYN_SITE_SSH:$REMOTE_PATH${DRY:+ (dry run)}"
echo "Files on the server that the build doesn't have are deleted (except .well-known)."
read -r -p "Continue? [y/N] " answer
[[ "$answer" == "y" || "$answer" == "Y" ]] || { echo "Cancelled."; exit 0; }

# Pages are world-readable files, folders 755. Set here rather than with rsync's
# --chmod, which the rsync that ships with macOS (openrsync) doesn't have.
chmod -R u=rwX,go=rX "$SITE/dist"
# --delete keeps the server an exact copy of the build; .well-known is left alone
# for Let's Encrypt. -rlptz rather than -a: the server keeps its own owner and group
# (HestiaCP serves the folder through the www-data group).
rsync -rlptz --delete $DRY ${DRY:+-v} --exclude=".well-known" \
      -e "ssh -p $PORT" "$SITE/dist/" "$OBDARYN_SITE_SSH:$REMOTE_PATH"
echo "Done: https://obdaryn.eu/"
