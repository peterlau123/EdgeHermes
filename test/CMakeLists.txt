cmake_minimum_required(VERSION 3.14...3.22)

project(NovaLLMTests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

# Use CMAKE_CURRENT_LIST_DIR for reliable path resolution
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
include(${PROJECT_ROOT}/cmake/tools.cmake OPTIONAL)

# ---- Dependencies ----

# Find NovaLLM package
find_package(NovaLLM REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/conan/include
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    NovaLLM::NovaLLM
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    fmt::fmt
    spdlog::spdlog
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
  endif()
endif()

# ---- Enable testing ----

enable_testing()
include(GoogleTest)

# On Windows, skip test discovery at build time to avoid DLL path issues
# Tests can still be run manually with ctest after ensuring DLLs are in PATH
if(NOT WIN32)
  gtest_discover_tests(${PROJECT_NAME})
else()
  message(STATUS "Skipping gtest_discover_tests on Windows (use 'ctest' to run tests manually)")
endif()

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(${PROJECT_NAME} PRIVATE -fprofile-arcs -ftest-coverage)
endif()
