#!/usr/bin/env bash
set -euo pipefail

# This script is deprecated. Use the unified cross-platform scripts instead:
#   scripts/build_macos.sh, scripts/build_ubuntu.sh, scripts/build_windows.ps1
# or the wrapper:
#   scripts/build.sh [--type Release|Debug] [--enable-logging ON|OFF] [--with-tests]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "[standalone/build.sh] Deprecated. Redirecting to scripts/build.sh ..."
exec "$REPO_ROOT/scripts/build.sh" "$@"
