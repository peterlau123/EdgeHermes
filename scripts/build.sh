#!/usr/bin/env bash
set -euo pipefail

# scripts/build.sh â€?Cross-platform build wrapper
# Unified flags:
#   --type Release|Debug
#   --enable-logging ON|OFF
#   --with-tests
#   --install-prefix <path>
#
# Dispatches to:
#   macOS:   scripts/build_macos.sh
#   Linux:   scripts/build_ubuntu.sh
#   Windows: scripts/build_windows.ps1 (via pwsh/powershell)

TYPE="Release"
ENABLE_LOGGING="ON"
WITH_TESTS="OFF"
INSTALL_PREFIX=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --type) TYPE="${2:-Release}"; shift 2 ;;
    --enable-logging) ENABLE_LOGGING="${2:-ON}"; shift 2 ;;
    --with-tests) WITH_TESTS="ON"; shift 1 ;;
    --install-prefix) INSTALL_PREFIX="${2:-}"; shift 2 ;;
    -h|--help)
      cat <<EOF
Usage: scripts/build.sh [--type Release|Debug] [--enable-logging ON|OFF] [--install-prefix <path>] [--with-tests]
EOF
      exit 0 ;;
    *) echo "Unknown arg: $1"; exit 1 ;;
  esac
done

uname_s=$(uname -s || echo unknown)
case "$uname_s" in
  Darwin)
    exec "$(dirname "$0")/build_macos.sh" \
      --type "$TYPE" \
      --enable-logging "$ENABLE_LOGGING" \
      ${WITH_TESTS:+--with-tests} \
      ${INSTALL_PREFIX:+--install-prefix "$INSTALL_PREFIX"}
    ;;
  Linux)
    exec "$(dirname "$0")/build_ubuntu.sh" \
      --type "$TYPE" \
      --enable-logging "$ENABLE_LOGGING" \
      ${WITH_TESTS:+--with-tests} \
      ${INSTALL_PREFIX:+--install-prefix "$INSTALL_PREFIX"}
    ;;
  MINGW*|MSYS*|CYGWIN*)
    if command -v pwsh >/dev/null 2>&1; then PS=pwsh; elif command -v powershell >/dev/null 2>&1; then PS=powershell; else echo "Error: PowerShell (pwsh or powershell) is required"; exit 1; fi
    WIN_ARGS=("-File" "$(dirname "$0")/build_windows.ps1" "-Configuration" "$TYPE" "-EnableLogging" "$ENABLE_LOGGING")
    [[ "$WITH_TESTS" == "ON" ]] && WIN_ARGS+=("-WithTests")
    [[ -n "$INSTALL_PREFIX" ]] && WIN_ARGS+=("-InstallPrefix" "$INSTALL_PREFIX")
    exec "$PS" -NoProfile -ExecutionPolicy Bypass ${WIN_ARGS[@]}
    ;;
  *)
    echo "Unsupported OS: $uname_s"; exit 1 ;;
 esac




