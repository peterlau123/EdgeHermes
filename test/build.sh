#!/bin/bash
set -e

# Create build directory
mkdir -p build
cd build

# Install dependencies with Conan
conan install .. --build=missing

# Configure and build with CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TEST_COVERAGE=ON
cmake --build .

# Run tests
ctest --output-on-failure

echo "Build and tests completed successfully!" 


