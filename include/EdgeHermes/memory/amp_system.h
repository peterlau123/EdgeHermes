#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "EdgeHermes/utils/macros.h"

namespace edgehermes {
namespace amp {

/**
 * @brief Base interface for memory allocators
 * 
 * This interface allows pluggable third-party allocators like tcmalloc,
 * jemalloc, and mimalloc to be integrated into the system.
 */
class edgehermes_API IMemoryAllocator {
 public:
  virtual ~IMemoryAllocator() = default;

  /**
   * @brief Allocate memory of specified size
   * @param size Size in bytes to allocate
   * @return Pointer to allocated memory, or nullptr on failure
   */
  virtual void* Allocate(size_t size) = 0;

  /**
   * @brief Deallocate previously allocated memory
   * @param ptr Pointer to memory to deallocate
   */
  virtual void Deallocate(void* ptr) = 0;

  /**
   * @brief Allocate memory with specific alignment
   * @param size Size in bytes to allocate
   * @param alignment Alignment requirement (must be power of 2)
   * @return Pointer to aligned memory, or nullptr on failure
   */
  virtual void* AllocateAligned(size_t size, size_t alignment) = 0;

  /**
   * @brief Get allocator name for debugging
   * @return Name string of the allocator implementation
   */
  virtual const char* Name() const = 0;
};

/**
 * @brief Allocator type enumeration
 */
enum class AllocatorType : uint8_t {
  STANDARD = 0,   // std::malloc/free
  TCMALLOC = 1,   // Google TCMalloc
  JEMALLOC = 2,   // jemalloc
  MIMALLOC = 3,   // Microsoft mimalloc
};

/**
 * @brief Configuration options for the AMP system
 */
struct edgehermes_API AMPConfig {
  AllocatorType allocator_type = AllocatorType::STANDARD;
  
  // Thread cache settings
  size_t thread_cache_size_kb = 512;      // Per-thread cache size in KB
  size_t central_cache_limit_mb = 128;    // Central cache size limit in MB
  
  // Performance settings
  bool numa_aware = false;                // Enable NUMA-aware allocation
  size_t max_cache_threads = 64;          // Max threads with caches
  
  // Monitoring settings
  bool enable_stats = false;
  double sample_rate = 0.01;              // Sample rate for profiling (1%)
  
  // Allocator-specific options
  std::unordered_map<std::string, std::string> allocator_options;
};

/**
 * @brief Memory statistics structure
 */
struct edgehermes_API MemoryStats {
  size_t total_allocated = 0;
  size_t active_allocations = 0;
  double fragmentation_ratio = 0.0;
  
  struct ThreadStats {
    size_t hits = 0;
    size_t misses = 0;
    size_t cache_size = 0;
  };
};

using IMemoryAllocatorPtr = std::unique_ptr<IMemoryAllocator>;
using IMemoryAllocatorSharedPtr = std::shared_ptr<IMemoryAllocator>;

}  // namespace amp
}  // namespace edgehermes



