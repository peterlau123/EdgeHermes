#!/usr/bin/env bash

# ============================================================================
# Peregrine Build Script for macOS and Linux
# ============================================================================
# This script provides a unified interface for building the Peregrine project
# with support for different build types, configurations, and targets.
#
# Usage: ./build.sh [options]
# Run with --help for detailed usage information
# ============================================================================

set -e  # Exit on error
set -o pipefail  # Catch errors in pipes

# ============================================================================
# Configuration and Defaults
# ============================================================================

# Build configuration
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_DIR="install"
ENABLE_LOGGING="ON"
CLEAN_BUILD=false

# Build targets
BUILD_MAIN=true
BUILD_TESTS=false
BUILD_STANDALONE=false
CREATE_PACKAGE=false

# Conan options
CONAN_BUILD_MISSING=true

# Script directory (for relative path resolution)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ============================================================================
# Color Output Functions
# ============================================================================

# Check if terminal supports colors
if [[ -t 1 ]] && command -v tput &> /dev/null && tput colors &> /dev/null; then
    COLOR_SUPPORT=true
else
    COLOR_SUPPORT=false
fi

print_header() {
    if [[ "$COLOR_SUPPORT" = true ]]; then
        echo -e "\n\033[1;36m==>\033[0m \033[1m$1\033[0m"
    else
        echo -e "\n==> $1"
    fi
}

print_success() {
    if [[ "$COLOR_SUPPORT" = true ]]; then
        echo -e "\033[1;32mâœ“\033[0m $1"
    else
        echo "âœ?$1"
    fi
}

print_info() {
    if [[ "$COLOR_SUPPORT" = true ]]; then
        echo -e "\033[1;34mâ†’\033[0m $1"
    else
        echo "â†?$1"
    fi
}

print_warning() {
    if [[ "$COLOR_SUPPORT" = true ]]; then
        echo -e "\033[1;33mâš \033[0m $1"
    else
        echo "âš?$1"
    fi
}

print_error() {
    if [[ "$COLOR_SUPPORT" = true ]]; then
        echo -e "\033[1;31mâœ?Error:\033[0m $1" >&2
    else
        echo "âœ?Error: $1" >&2
    fi
}

# ============================================================================
# Help and Usage
# ============================================================================

show_help() {
    cat << EOF
Peregrine Build Script

Usage: $0 [options]

Build Configuration:
  -r, --release           Build in Release mode (default)
  -d, --debug             Build in Debug mode
  -l, --logging           Enable logging (default: ON)
  --no-logging            Disable logging

Build Targets:
  -m, --main              Build main project (default)
  -t, --tests             Build and run tests
  -s, --standalone        Build standalone application
  -p, --package           Create Conan package
  -a, --all               Build everything (main + tests + standalone + package)

Build Options:
  -c, --clean             Clean all build-* and install-* directories (including custom
                          directories specified via --build-dir/--install-dir) before building
  --build-dir DIR         Set custom build directory (default: build)
  --install-dir DIR       Set custom install directory (default: install)

Other Options:
  -h, --help              Show this help message
  -v, --verbose           Enable verbose output

Examples:
  $0                      # Build main project in Release mode
  $0 -d -t                # Build and run tests in Debug mode
  $0 -r -a                # Build everything in Release mode
  $0 -c -r -m             # Clean build main project in Release mode

EOF
}

# ============================================================================
# Command Line Argument Parsing
# ============================================================================

VERBOSE=false

parse_args() {
    # Reset target flags if any target is explicitly specified
    local targets_specified=false
    local main_explicitly_specified=false
    
    # First pass: check if any targets are specified
    for arg in "$@"; do
        case $arg in
            -m|--main)
                main_explicitly_specified=true
                ;;
            -t|--tests|-s|--standalone|-p|--package|-a|--all)
                targets_specified=true
                ;;
        esac
    done
    
    # If targets are specified but main is not explicitly requested, disable default main build
    # We'll check package freshness later to decide if main needs to be built
    if [[ "$targets_specified" = true && "$main_explicitly_specified" = false ]]; then
        BUILD_MAIN=false
    fi
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -l|--logging)
                ENABLE_LOGGING="ON"
                shift
                ;;
            --no-logging)
                ENABLE_LOGGING="OFF"
                shift
                ;;
            -m|--main)
                BUILD_MAIN=true
                shift
                ;;
            -t|--tests)
                BUILD_TESTS=true
                shift
                ;;
            -s|--standalone)
                BUILD_STANDALONE=true
                shift
                ;;
            -p|--package)
                CREATE_PACKAGE=true
                shift
                ;;
            -a|--all)
                BUILD_MAIN=true
                BUILD_TESTS=true
                BUILD_STANDALONE=true
                CREATE_PACKAGE=true
                shift
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            --build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            --install-dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # Adjust build directory for Debug builds
    if [[ "$BUILD_TYPE" == "Debug" ]] && [[ "$BUILD_DIR" == "build" ]]; then
        BUILD_DIR="build-debug"
        INSTALL_DIR="install-debug"
    fi
}

# ============================================================================
# Dependency Checks
# ============================================================================

check_dependencies() {
    print_header "Checking dependencies"
    
    local missing_deps=()
    
    # Check for required commands
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    else
        print_info "CMake: $(cmake --version | head -n1)"
    fi
    
    if ! command -v conan &> /dev/null; then
        missing_deps+=("conan")
    else
        print_info "Conan: $(conan --version)"
    fi
    
    if ! command -v python3 &> /dev/null && ! command -v python &> /dev/null; then
        missing_deps+=("python")
    fi
    
    # Report missing dependencies
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_error "Missing required dependencies: ${missing_deps[*]}"
        echo
        echo "Please install the missing dependencies:"
        for dep in "${missing_deps[@]}"; do
            case $dep in
                cmake)
                    echo "  - CMake: https://cmake.org/download/"
                    ;;
                conan)
                    echo "  - Conan: pip install conan"
                    ;;
                python)
                    echo "  - Python: https://www.python.org/downloads/"
                    ;;
            esac
        done
        exit 1
    fi
    
    print_success "All dependencies satisfied"
}

# ============================================================================
# Conan Setup
# ============================================================================

setup_conan() {
    print_header "Setting up Conan"
    
    # Detect Conan profile if it doesn't exist
    if ! conan profile show default &> /dev/null; then
        print_info "Detecting Conan profile..."
        conan profile detect --force
    else
        print_info "Using existing Conan profile"
    fi
}

# ============================================================================
# Package Detection Functions
# ============================================================================

check_package_exists() {
    local package_ref="Peregrine/0.1.0@local/testing"
    if conan list "$package_ref" 2>/dev/null | grep -q "$package_ref"; then
        return 0  # Package exists
    else
        return 1  # Package doesn't exist
    fi
}

get_package_timestamp() {
    local package_ref="Peregrine/0.1.0@local/testing"
    # Get the package folder path from conan cache
    local cache_info
    cache_info=$(conan cache path "$package_ref" 2>/dev/null)
    
    if [[ -z "$cache_info" ]]; then
        echo "0"  # Package doesn't exist
        return
    fi
    
    # Get the most recent modification time in the package
    local package_time
    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS
        package_time=$(stat -f "%m" "$cache_info" 2>/dev/null || echo "0")
    else
        # Linux
        package_time=$(stat -c "%Y" "$cache_info" 2>/dev/null || echo "0")
    fi
    
    echo "$package_time"
}

get_source_timestamp() {
    # Get the most recent modification time of source files
    local newest_time=0
    local file
    
    # Check source, include, and CMakeLists.txt files
    while IFS= read -r -d '' file; do
        local file_time
        if [[ "$(uname)" == "Darwin" ]]; then
            file_time=$(stat -f "%m" "$file" 2>/dev/null || echo "0")
        else
            file_time=$(stat -c "%Y" "$file" 2>/dev/null || echo "0")
        fi
        
        if [[ $file_time -gt $newest_time ]]; then
            newest_time=$file_time
        fi
    done < <(find source include CMakeLists.txt conanfile.py -type f -print0 2>/dev/null)
    
    echo "$newest_time"
}

is_package_outdated() {
    if ! check_package_exists; then
        print_info "Package not found in cache"
        return 0  # Package doesn't exist, needs rebuild
    fi
    
    local package_time
    local source_time
    
    package_time=$(get_package_timestamp)
    source_time=$(get_source_timestamp)
    
    if [[ $source_time -gt $package_time ]]; then
        print_info "Source code is newer than cached package"
        return 0  # Source is newer, needs rebuild
    else
        print_info "Cached package is up-to-date"
        return 1  # Package is up-to-date
    fi
}

# ============================================================================
# Build Functions
# ============================================================================

clean_build_dirs() {
    print_header "Cleaning build directories"
    
    local cleaned=false
    local dirs_to_clean=()
    
    # First, add user-specified directories if they exist
    if [[ -n "$BUILD_DIR" && -d "$BUILD_DIR" ]]; then
        dirs_to_clean+=("$BUILD_DIR")
    fi
    if [[ -n "$INSTALL_DIR" && -d "$INSTALL_DIR" ]]; then
        dirs_to_clean+=("$INSTALL_DIR")
    fi
    
    # Then, find all build and install directories
    shopt -s nullglob  # Don't include pattern if no match
    for dir in build build-* build_* install install-* install_*; do
        if [[ -d "$dir" ]]; then
            # Avoid duplicates
            local already_added=false
            for added_dir in "${dirs_to_clean[@]}"; do
                if [[ "$dir" == "$added_dir" ]]; then
                    already_added=true
                    break
                fi
            done
            if [[ "$already_added" == false ]]; then
                dirs_to_clean+=("$dir")
            fi
        fi
    done
    shopt -u nullglob
    
    # Remove all collected directories
    for dir in "${dirs_to_clean[@]}"; do
        print_info "Removing $dir"
        rm -rf "$dir"
        cleaned=true
    done
    
    if [[ "$cleaned" == false ]]; then
        print_info "No build or install directories to clean"
    fi
    
    print_success "Clean complete"
}

install_dependencies() {
    local target_dir="$1"
    local build_tests="$2"
    
    print_info "Installing Conan dependencies..."
    
    local conan_opts=(
        --output-folder=.
        -s build_type="$BUILD_TYPE"
    )
    
    if [[ "$build_tests" == "True" ]]; then
        conan_opts+=(-o build_tests=True)
    fi
    
    if [[ "$CONAN_BUILD_MISSING" == true ]]; then
        conan_opts+=("--build=missing")
    fi
    
    if [[ "$VERBOSE" == true ]]; then
        conan install .. "${conan_opts[@]}"
    else
        conan install .. "${conan_opts[@]}" > /dev/null
    fi
}

find_toolchain_file() {
    local toolchain_file
    toolchain_file=$(find "$(pwd)" -name "conan_toolchain.cmake" -type f | head -1)
    
    if [[ -z "$toolchain_file" ]]; then
        print_error "Could not find conan_toolchain.cmake"
        exit 1
    fi
    
    echo "$toolchain_file"
}

build_main_project() {
    print_header "Building main project ($BUILD_TYPE)"
    
    # Create build directory
    mkdir -p "$BUILD_DIR" "$INSTALL_DIR"
    cd "$BUILD_DIR"
    
    # Install dependencies
    local build_tests="False"
    if [[ "$BUILD_TESTS" == true ]]; then
        build_tests="True"
    fi
    install_dependencies "." "$build_tests"
    
    # Find toolchain file
    local toolchain_file
    toolchain_file=$(find_toolchain_file)
    print_info "Using toolchain: $toolchain_file"
    
    # Configure
    print_info "Configuring CMake..."
    cmake -S .. -B . \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -Dperegrine_ENABLE_LOGGING="$ENABLE_LOGGING" \
        -DCMAKE_INSTALL_PREFIX="../$INSTALL_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain_file"
    
    # Build
    print_info "Building..."
    cmake --build . --config "$BUILD_TYPE" ${VERBOSE:+--verbose}
    
    # Install
    print_info "Installing..."
    cmake --install . --config "$BUILD_TYPE"
    
    cd ..
    print_success "Main project build complete"
}

build_tests() {
    print_header "Building and running tests (Debug)"
    
    local test_build_dir="build-test"
    
    # Create build directory
    mkdir -p "$test_build_dir"
    cd "$test_build_dir"
    
    # Install dependencies
    print_info "Installing test dependencies..."
    mkdir -p conan
    if [[ "$VERBOSE" == true ]]; then
        conan install ../test --output-folder=conan --build=missing -s build_type=Debug
    else
        conan install ../test --output-folder=conan --build=missing -s build_type=Debug > /dev/null
    fi
    
    # Find toolchain file
    local toolchain_file
    toolchain_file=$(find_toolchain_file)
    print_info "Using toolchain: $toolchain_file"
    
    # Configure
    print_info "Configuring test CMake..."
    cmake -S ../test -B . \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain_file"
    
    # Build
    print_info "Building tests..."
    cmake --build . --config Debug ${VERBOSE:+--verbose}
    
    # Run tests if available
    if [[ -f ./CTestTestfile.cmake ]]; then
        print_info "Running tests..."
        ctest --output-on-failure || print_warning "Some tests failed"
    else
        print_warning "No CTest configuration found, skipping test execution"
    fi
    
    cd ..
    print_success "Test build complete"
}

build_standalone() {
    print_header "Building standalone application ($BUILD_TYPE)"
    
    local standalone_build_dir="build-standalone"
    
    # Create build directory
    mkdir -p "$standalone_build_dir"
    cd "$standalone_build_dir"
    
    # Install dependencies
    print_info "Installing standalone dependencies..."
    mkdir -p conan
    if [[ "$VERBOSE" == true ]]; then
        conan install ../standalone --output-folder=conan --build=missing -s build_type="$BUILD_TYPE"
    else
        conan install ../standalone --output-folder=conan --build=missing -s build_type="$BUILD_TYPE" > /dev/null
    fi
    
    # Find toolchain file
    local toolchain_file
    toolchain_file=$(find_toolchain_file)
    print_info "Using toolchain: $toolchain_file"
    
    # Configure
    print_info "Configuring standalone CMake..."
    cmake -S ../standalone -B . \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain_file"
    
    # Build
    print_info "Building standalone..."
    cmake --build . --config "$BUILD_TYPE" ${VERBOSE:+--verbose}
    
    cd ..
    print_success "Standalone build complete"
}

create_conan_package() {
    print_header "Creating Conan package ($BUILD_TYPE)"
    
    print_info "Running conan create..."
    if [[ "$VERBOSE" == true ]]; then
        conan create . --user=local --channel=testing --build=missing -s build_type="$BUILD_TYPE"
    else
        conan create . --user=local --channel=testing --build=missing -s build_type="$BUILD_TYPE" > /dev/null
    fi
    
    print_success "Conan package created"
}

# ============================================================================
# Build Summary
# ============================================================================

print_build_summary() {
    print_header "Build Configuration Summary"
    echo "  Build Type:      $BUILD_TYPE"
    echo "  Logging:         $ENABLE_LOGGING"
    echo "  Build Directory: $BUILD_DIR"
    echo "  Install Dir:     $INSTALL_DIR"
    echo
    echo "  Build Targets:"
    [[ "$BUILD_MAIN" == true ]] && echo "    âœ?Main project"
    [[ "$BUILD_TESTS" == true ]] && echo "    âœ?Tests"
    [[ "$BUILD_STANDALONE" == true ]] && echo "    âœ?Standalone"
    [[ "$CREATE_PACKAGE" == true ]] && echo "    âœ?Conan package"
    echo
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    # Parse command line arguments
    parse_args "$@"
    
    # Print summary
    print_build_summary
    
    # Check dependencies
    check_dependencies
    
    # Setup Conan
    setup_conan
    
    # Clean if requested
    if [[ "$CLEAN_BUILD" == true ]]; then
        clean_build_dirs
    fi
    
    # Execute build targets
    # If tests or standalone are requested but main wasn't explicitly requested,
    # check if we need to rebuild main based on package freshness
    local need_package=false
    if [[ "$BUILD_TESTS" == true || "$BUILD_STANDALONE" == true ]]; then
        need_package=true
        if [[ "$BUILD_MAIN" == false ]]; then
            print_header "Checking package freshness"
            if is_package_outdated; then
                print_info "Enabling main project build due to outdated/missing package"
                BUILD_MAIN=true
            fi
        fi
    fi
    
    if [[ "$BUILD_MAIN" == true ]]; then
        build_main_project
        
        # Auto-create package if tests or standalone need it
        if [[ "$need_package" == true || "$CREATE_PACKAGE" == true ]]; then
            print_info "Creating/updating Conan package for downstream consumers..."
            CREATE_PACKAGE=true
        fi
    fi
    
    if [[ "$CREATE_PACKAGE" == true ]]; then
        create_conan_package
    fi
    
    if [[ "$BUILD_TESTS" == true ]]; then
        build_tests
    fi
    
    if [[ "$BUILD_STANDALONE" == true ]]; then
        build_standalone
    fi
    
    # Final success message
    echo
    print_success "All builds completed successfully! ðŸŽ‰"
    echo
}

# Run main function with all arguments
main "$@"




