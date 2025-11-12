cmake_minimum_required(VERSION 3.23)

# ===================== Project Options =====================
include(cmake/options.cmake)

# ===================== Project Info ========================
project(
  NovaLLM
  VERSION 0.1.0
  DESCRIPTION "A lightweight and efficient C/C++ library for Large Language Model (LLM) inference"
  LANGUAGES CXX C
  HOMEPAGE_URL  "https://github.com/peterlau123/NovaLLM"
)

# ===================== Conan Dependencies ==================
# Find packages managed by Conan
find_package(fmt REQUIRED)
if(NOVA_LLM_ENABLE_LOGGING)
  find_package(spdlog REQUIRED)
endif()
# ===================== Build Guards ========================
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed. Please use a build directory.")
endif()

# ===================== Source Files ========================
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ===================== Build Type ==========================
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# ===================== Documentation =======================
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(docs
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM
  )
endif()

# ===================== Library ============================
add_library(${PROJECT_NAME} SHARED ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
if(WIN32)
# Define export macro when building the library so Windows builds use __declspec(dllexport)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOVA_LLM_EXPORTS)
endif()

# ===================== Link Dependencies ===================
set(DEPENDENCY_LIST fmt::fmt)
if(NOVA_LLM_ENABLE_LOGGING)
  list(APPEND DEPENDENCY_LIST spdlog::spdlog)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPENDENCY_LIST})

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# ===================== Install Targets =====================
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "默认安装路径" FORCE)
  message(STATUS "设置安装路径为: ${CMAKE_INSTALL_PREFIX}")
endif()

# Install library and headers
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(DIRECTORY include/
        DESTINATION include/
        FILES_MATCHING PATTERN "*.h"
)

# Generate and install config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME}
)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION lib/cmake/${PROJECT_NAME}
)



# ===================== Summary =============================
message(STATUS "\n==== NovaLLM Configuration Summary ====")
message(STATUS "  Logging:         ${NOVA_LLM_ENABLE_LOGGING}")
message(STATUS "  Build tests:     ${NOVA_LLM_BUILD_TESTS}")
message(STATUS "  Install prefix:  ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C++ Standard:    17")
message(STATUS "  Dependencies:     fmt, spdlog")
message(STATUS "====================================\n")
