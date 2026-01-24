#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#define PEREGRINE_VERSION_MAJOR 0
#define PEREGRINE_VERSION_MINOR 1
#define PEREGRINE_VERSION_PATCH 0
#define PEREGRINE_VERSION_STRING "0.1.0"
#define PEREGRINE_VERSION (PEREGRINE_VERSION_MAJOR * 10000 + PEREGRINE_VERSION_MINOR * 100 + PEREGRINE_VERSION_PATCH)

// For API export and import
#if defined(_WIN32)
// When building the library define PEREGRINE_EXPORTS (set by CMake)
#if defined(PEREGRINE_EXPORTS)
#define PEREGRINE_API __declspec(dllexport)
#else
#define PEREGRINE_API __declspec(dllimport)
#endif
#else
#define PEREGRINE_API __attribute__((visibility("default")))
#endif

// For debugging and runtime check
#ifdef NDEBUG
#define ASSERT(condition, message) ((void)0)
#else
#define ASSERT(condition, message)                                                                      \
  do {                                                                                                  \
    if (!(condition)) {                                                                                 \
      throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + message); \
    }                                                                                                   \
  } while (0)
#endif


// API markers
#define _IN
#define _OUT
#define _INOUT

namespace peregrine {

template <typename T>
using SharedPtr = std::shared_ptr<T>;

}  // namespace peregrine
