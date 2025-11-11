#!/bin/bash

# NovaLLM build script
# Usage: ./build.sh [options]
# Options:
#   -d, --debug     Build in debug mode
#   -r, --release   Build in release mode (default)
#   -t, --tests     Build with tests enabled
#   -l, --logging   Enable logging (default)
#   -c, --clean     Clean build directory before building
#   -h, --help      Show this help message

# Default values
BUILD_TYPE="Release"  # 改为大写的Release
BUILD_DIR="build"
ENABLE_TESTS="OFF"
ENABLE_LOGGING="ON"
CLEAN_BUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"    # 改为大写的Debug
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"  # 改为大写的Release
            shift
            ;;
        -t|--tests)
            ENABLE_TESTS="ON"
            shift
            ;;
        -l|--logging)
            ENABLE_LOGGING="ON"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  -d, --debug     Build in debug mode"
            echo "  -r, --release   Build in release mode (default)"
            echo "  -t, --tests     Build with tests enabled"
            echo "  -l, --logging   Enable logging (default)"
            echo "  -c, --clean     Clean build directory before building"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Function to print colored messages
print_message() {
    local color=$1
    local message=$2
    case $color in
        "green") echo -e "\033[32m$message\033[0m" ;;
        "yellow") echo -e "\033[33m$message\033[0m" ;;
        "red") echo -e "\033[31m$message\033[0m" ;;
        *) echo "$message" ;;
    esac
}

# Function to check if a command exists
check_command() {
    if ! command -v $1 &> /dev/null; then
        print_message "red" "Error: $1 is required but not installed."
        exit 1
    fi
}

# Check required tools
check_command cmake
check_command conan

# Create or clean build directory
if [ "$CLEAN_BUILD" = true ]; then
    print_message "yellow" "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

if [  -d "install" ]; then
    print_message "yellow" "Cleaning install directory..."
    rm -rf "install"
fi

if [ ! -d "$BUILD_DIR" ]; then
    print_message "green" "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi


# CMakePresets.json is no longer used - all builds use direct cmake commands

# Print build configuration
print_message "green" "\nBuild Configuration:"
echo "  Build Type:   $BUILD_TYPE"
echo "  Build Tests:  $ENABLE_TESTS"
echo "  Logging:      $ENABLE_LOGGING"
echo "  Build Dir:    $BUILD_DIR"
echo

# Change to build directory
cd "$BUILD_DIR" || exit 1

# Install dependencies with Conan
print_message "green" "Installing dependencies..."

# Convert CMake ON/OFF to Conan True/False
if [ "$ENABLE_TESTS" = "ON" ]; then
    CONAN_BUILD_TESTS="True"
else
    CONAN_BUILD_TESTS="False"
fi

if ! conan install .. --output-folder=. --build=missing -s build_type="$BUILD_TYPE" -o build_tests="$CONAN_BUILD_TESTS"; then
    print_message "red" "Failed to install dependencies"
    exit 1
fi

# Source Conan environment
print_message "green" "Setting up Conan environment..."
source build/${BUILD_TYPE}/generators/conanbuild.sh

# Configure with CMake using presets
print_message "green" "Configuring project..."
if ! cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DNOVA_LLM_ENABLE_LOGGING="$ENABLE_LOGGING" \
    -DCMAKE_TOOLCHAIN_FILE=build/$BUILD_TYPE/generators/conan_toolchain.cmake; then
    print_message "red" "CMake configuration failed"
    exit 1
fi

# Build the project using presets
print_message "green" "Building project..."
if ! cmake --build . --config $BUILD_TYPE; then    # 移除 ${BUILD_TYPE^}
    print_message "red" "Build failed"
    exit 1
fi

# Install the project
print_message "green" "Installing project..."
if ! cmake --install . --config $BUILD_TYPE; then  # 移除 ${BUILD_TYPE^}
    print_message "red" "Installation failed"
    exit 1
fi

print_message "green" "\nBuild completed successfully!"