#!/usr/bin/env bash
set -euo pipefail

# build_macos.sh â€?Build Peregrine on macOS using Conan + CMake
# Mirrors the CI steps in .github/workflows/macos.yml
#
# Usage:
#   scripts/build_macos.sh [--type Release|Debug] [--enable-logging ON|OFF] [--install-prefix <path>] [--with-tests]
# Examples:
#   scripts/build_macos.sh
#   scripts/build_macos.sh --type Debug --with-tests
#   scripts/build_macos.sh --install-prefix "$PWD/install" --enable-logging OFF

TYPE="Release"
ENABLE_LOGGING="ON"
WITH_TESTS="OFF"
INSTALL_PREFIX=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --type)
      TYPE="${2:-Release}"; shift 2 ;;
    --enable-logging)
      ENABLE_LOGGING="${2:-ON}"; shift 2 ;;
    --with-tests)
      WITH_TESTS="ON"; shift 1 ;;
    --install-prefix)
      INSTALL_PREFIX="${2:-}"; shift 2 ;;
    -h|--help)
      echo "Usage: scripts/build_macos.sh [--type Release|Debug] [--enable-logging ON|OFF] [--install-prefix <path>] [--with-tests]"; exit 0 ;;
    *) echo "Unknown arg: $1"; exit 1 ;;
  esac
done

# Requirements check
need() { command -v "$1" >/dev/null 2>&1 || { echo "Error: '$1' is required"; exit 1; }; }
need python3
need cmake
need conan

# Detect conan profile to ensure a working toolchain
conan profile detect --force

# Layout
BUILD_DIR="build${TYPE:+-$(echo "$TYPE" | tr 'A-Z' 'a-z')}"
INSTALL_DIR="install${TYPE:+-$(echo "$TYPE" | tr 'A-Z' 'a-z')}"
[[ -n "$INSTALL_PREFIX" ]] && INSTALL_DIR="$INSTALL_PREFIX"

mkdir -p "$BUILD_DIR" "$INSTALL_DIR"

pushd "$BUILD_DIR" >/dev/null

# Install deps via Conan and generate toolchain
# If tests are requested, pull test deps too for the main build configuration
CONAN_BUILD_TESTS="False"
[[ "$WITH_TESTS" == "ON" ]] && CONAN_BUILD_TESTS="True"

conan install .. \
  --output-folder=. \
  --build=missing \
  -s build_type="$TYPE" \
  -o build_tests="$CONAN_BUILD_TESTS"

# Find toolchain
TOOLCHAIN_FILE=$(find "$(pwd)" -name "conan_toolchain.cmake" -type f | head -1 || true)
[[ -z "$TOOLCHAIN_FILE" ]] && { echo "conan_toolchain.cmake not found"; exit 1; }

echo "Using toolchain: $TOOLCHAIN_FILE"

cmake -S .. -B . \
  -DCMAKE_BUILD_TYPE="$TYPE" \
  -Dperegrine_ENABLE_LOGGING="$ENABLE_LOGGING" \
  -DCMAKE_INSTALL_PREFIX="$(cd .. && realpath "$INSTALL_DIR")" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"

cmake --build . --config "$TYPE"
cmake --install . --config "$TYPE"

popd >/dev/null

echo "Build complete (type=$TYPE, logging=$ENABLE_LOGGING). Artifacts installed to '$INSTALL_DIR'" 

# Optional: build and run tests as a separate CMake project
if [[ "$WITH_TESTS" == "ON" ]]; then
  echo "Building tests..."
  TEST_BUILD_DIR="build-test${TYPE:+-$(echo "$TYPE" | tr 'A-Z' 'a-z')}"
  mkdir -p "$TEST_BUILD_DIR"
  pushd "$TEST_BUILD_DIR" >/dev/null

  conan install ../test --output-folder=conan --build=missing -s build_type="$TYPE"
  TOOLCHAIN_FILE=$(find "$(pwd)" -name "conan_toolchain.cmake" -type f | head -1 || true)
  [[ -z "$TOOLCHAIN_FILE" ]] && { echo "conan_toolchain.cmake (tests) not found"; exit 1; }

  cmake -S ../test -B . -DCMAKE_BUILD_TYPE="$TYPE" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
  cmake --build . --config "$TYPE"
  if command -v ctest >/dev/null 2>&1; then
    ctest --output-on-failure || true
  fi
  popd >/dev/null
fi




