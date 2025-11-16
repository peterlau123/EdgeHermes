#!/usr/bin/env bash
set -euo pipefail

# build_ubuntu.sh — Build NovaLLM on Ubuntu using Conan + CMake
# Mirrors the CI steps in .github/workflows/ubuntu.yml
#
# Usage:
#   scripts/build_ubuntu.sh [--type Release|Debug] [--enable-logging ON|OFF] [--install-prefix <path>] [--with-tests]

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
    -h|--help) echo "Usage: scripts/build_ubuntu.sh [--type Release|Debug] [--enable-logging ON|OFF] [--install-prefix <path>] [--with-tests]"; exit 0 ;;
    *) echo "Unknown arg: $1"; exit 1 ;;
  esac
done

need() { command -v "$1" >/dev/null 2>&1 || { echo "Error: '$1' is required"; exit 1; }; }
need cmake
need conan
need python3

conan profile detect --force

BUILD_DIR="build${TYPE:+-$(echo "$TYPE" | tr 'A-Z' 'a-z')}"
INSTALL_DIR="install${TYPE:+-$(echo "$TYPE" | tr 'A-Z' 'a-z')}"
[[ -n "$INSTALL_PREFIX" ]] && INSTALL_DIR="$INSTALL_PREFIX"

mkdir -p "$BUILD_DIR" "$INSTALL_DIR"

pushd "$BUILD_DIR" >/dev/null
CONAN_BUILD_TESTS="False"; [[ "$WITH_TESTS" == "ON" ]] && CONAN_BUILD_TESTS="True"
conan install .. --output-folder=. --build=missing -s build_type="$TYPE" -o build_tests="$CONAN_BUILD_TESTS"
TOOLCHAIN_FILE=$(find "$(pwd)" -name "conan_toolchain.cmake" -type f | head -1 || true)
[[ -z "$TOOLCHAIN_FILE" ]] && { echo "conan_toolchain.cmake not found"; exit 1; }

cmake -S .. -B . \
  -DCMAKE_BUILD_TYPE="$TYPE" \
  -DNOVA_LLM_ENABLE_LOGGING="$ENABLE_LOGGING" \
  -DCMAKE_INSTALL_PREFIX="$(cd .. && realpath "$INSTALL_DIR")" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"

cmake --build . --config "$TYPE"
cmake --install . --config "$TYPE"

popd >/dev/null

echo "Build complete (type=$TYPE, logging=$ENABLE_LOGGING). Artifacts installed to '$INSTALL_DIR'"

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
