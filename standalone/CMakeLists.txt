cmake_minimum_required(VERSION 3.14...3.22)

project(PeregrineStandalone LANGUAGES CXX)

# --- Import tools ----

# Use CMAKE_CURRENT_LIST_DIR for reliable path resolution
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
include(${PROJECT_ROOT}/cmake/tools.cmake OPTIONAL)

# ---- Dependencies ----

# Find peregrine package and its dependencies
find_package(peregrine REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(cxxopts REQUIRED)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "peregrine")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    peregrine::peregrine
    fmt::fmt
    spdlog::spdlog
    cxxopts::cxxopts
)

# Enable compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
endif()
