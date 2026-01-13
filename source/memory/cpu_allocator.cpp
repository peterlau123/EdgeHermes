#include "EdgeHermes/memory/allocator.h"

#include <cstdlib>
#include <new>
#include <vector>

#ifdef edgehermes_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

// Third-party allocator headers
#ifdef edgehermes_ENABLE_TCMALLOC
#include <gperftools/tcmalloc.h>
#endif

#ifdef edgehermes_ENABLE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

#ifdef edgehermes_ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

#include "EdgeHermes/utils/log.h"

namespace edgehermes {
namespace amp {

// Helper function for aligned allocation
static void* AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;
  void* ptr = nullptr;
#if defined(_WIN32)
  ptr = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&ptr, alignment, size) != 0) {
    ptr = nullptr;
  }
#endif
  return ptr;
}

// Standard Allocator Implementation
void* StandardAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;
  return std::malloc(size);
}

void StandardAllocator::Deallocate(void* ptr) {
  if (ptr) std::free(ptr);
}

void* StandardAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;
  void* ptr = nullptr;
#if defined(_WIN32)
  ptr = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&ptr, alignment, size) != 0) {
    ptr = nullptr;
  }
#endif
  return ptr;
}

// TCMalloc Allocator Implementation
TCMallocAllocator::TCMallocAllocator(const std::unordered_map<std::string, std::string>& options) {
  // Configure TCMalloc with options if needed
  // TCMalloc typically uses environment variables for configuration
  // Options like max_cache_size, background_threads, etc. can be set via environment
  (void)options;  // Suppress unused parameter warning
}

void* TCMallocAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;

#ifdef edgehermes_ENABLE_TCMALLOC
  return tc_malloc(size);
#else
  return std::malloc(size);  // Fallback to standard malloc
#endif
}

void TCMallocAllocator::Deallocate(void* ptr) {
  if (!ptr) return;

#ifdef edgehermes_ENABLE_TCMALLOC
  tc_free(ptr);
#else
  std::free(ptr);  // Fallback to standard free
#endif
}

void* TCMallocAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;

#ifdef edgehermes_ENABLE_TCMALLOC
  // TCMalloc's tc_memalign may not be available in all versions
  // Use posix_memalign as fallback for TCMalloc builds
  return AllocateAligned(size, alignment);
#else
  return AllocateAligned(size, alignment);  // Fallback
#endif
}

// Jemalloc Allocator Implementation
JemallocAllocator::JemallocAllocator(const std::unordered_map<std::string, std::string>& options) {
  // Configure jemalloc with options via mallctl if needed
  // Options like narenas, dirty_decay_ms, etc. can be configured
  (void)options;  // Suppress unused parameter warning
}

void* JemallocAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;

#ifdef edgehermes_ENABLE_JEMALLOC
  return je_malloc(size);
#else
  return std::malloc(size);  // Fallback to standard malloc
#endif
}

void JemallocAllocator::Deallocate(void* ptr) {
  if (!ptr) return;

#ifdef edgehermes_ENABLE_JEMALLOC
  je_free(ptr);
#else
  std::free(ptr);  // Fallback to standard free
#endif
}

void* JemallocAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;

#ifdef edgehermes_ENABLE_JEMALLOC
  // jemalloc 5.0+ has je_aligned_alloc
  return je_aligned_alloc(alignment, size);
#else
  return AllocateAligned(size, alignment);  // Fallback
#endif
}

// Mimalloc Allocator Implementation
MimallocAllocator::MimallocAllocator(const std::unordered_map<std::string, std::string>& options) {
  // Configure mimalloc with options if needed
  // Options like heap_grow_factor, heap_max_size, etc. can be configured
  (void)options;  // Suppress unused parameter warning
}

void* MimallocAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;

#ifdef edgehermes_ENABLE_MIMALLOC
  return mi_malloc(size);
#else
  return std::malloc(size);  // Fallback to standard malloc
#endif
}

void MimallocAllocator::Deallocate(void* ptr) {
  if (!ptr) return;

#ifdef edgehermes_ENABLE_MIMALLOC
  mi_free(ptr);
#else
  std::free(ptr);  // Fallback to standard free
#endif
}

void* MimallocAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;

#ifdef edgehermes_ENABLE_MIMALLOC
  return mi_aligned_alloc(alignment, size);
#else
  return AllocateAligned(size, alignment);  // Fallback
#endif
}

}  // namespace amp
}  // namespace edgehermes




