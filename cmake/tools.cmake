# tools.cmake - Common CMake utilities and helper functions
# This file provides common utilities used across multiple CMakeLists.txt files

# Note: This file can be extended with common functions, macros, or configurations
# that are shared across the test, standalone, and other subdirectories.

# Currently, individual CMakeLists.txt files define their own compiler warnings,
# so this file serves as a placeholder for future common utilities.

# Example: Common compiler warning settings could be defined here if needed
# function(set_common_compiler_warnings target)
#     if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#         target_compile_options(${target} PRIVATE -Wall -Wpedantic -Wextra -Werror)
#     elseif(MSVC)
#         target_compile_options(${target} PRIVATE /W4 /WX)
#     endif()
# endfunction()

