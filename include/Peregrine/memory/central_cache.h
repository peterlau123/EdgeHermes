#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Peregrine/memory/allocator.h"

#include "Peregrine/utils/macros.h"
#include "Peregrine/memory/size_class.h"

namespace peregrine {
namespace amp {

/**
 * @brief Central cache for shared free lists per size class
 *
 * Manages free lists for each size class with low-contention locking.
 * Acts as an intermediary between thread caches and the page heap.
 */
class PEREGRINE_API CentralCache {
 public:
  /**
   * @brief Constructor
   * @param size_class_system Reference to the global size class system
   * @param max_cache_size_mb Maximum central cache size in MB
   */
  explicit CentralCache(const SizeClassSystem& size_class_system,
                       size_t max_cache_size_mb = 128);

  /**
   * @brief Destructor - returns all objects to page heap
   */
  ~CentralCache();

  /**
   * @brief Allocate a batch of objects from central cache
   * @param size_class Size class ID
   * @param count Number of objects to allocate
   * @return Vector of allocated objects (may be smaller than requested)
   */
  std::vector<void*> AllocateBatch(size_t size_class, size_t count);

  /**
   * @brief Deallocate a batch of objects to central cache
   * @param size_class Size class ID
   * @param objects Objects to deallocate
   */
  void DeallocateBatch(size_t size_class, const std::vector<void*>& objects);

  /**
   * @brief Get central cache statistics
   */
  struct CacheStats {
    size_t total_objects = 0;
    size_t total_bytes = 0;
    size_t cache_limit_mb = 0;
    std::array<size_t, SizeClassSystem::NUM_SIZE_CLASSES> objects_per_class{};
  };
  CacheStats GetStats() const;

  /**
   * @brief Check if cache is at capacity limit
   * @return true if cache should stop accepting more objects
   */
  bool IsAtCapacity() const;

 private:
  /**
   * @brief Per-size-class free list
   */
  struct SizeClassList {
    std::vector<void*> objects;
    mutable std::mutex mutex;
    size_t total_bytes = 0;
  };

  /**
   * @brief Refill size class list from page heap
   * @param size_class Size class ID
   * @param count Number of objects to allocate
   * @return Number of objects actually allocated
   */
  size_t RefillFromPageHeap(size_t size_class, size_t count);

  /**
   * @brief Return excess objects to page heap
   * @param size_class Size class ID
   */
  void ReturnToPageHeap(size_t size_class);

  // Member variables
  const SizeClassSystem& size_class_system_;
  std::array<SizeClassList, SizeClassSystem::NUM_SIZE_CLASSES> size_class_lists_;
  size_t max_cache_size_mb_;
  std::atomic<size_t> current_cache_size_mb_{0};

  // Disable copy and move
  CentralCache(const CentralCache&) = delete;
  CentralCache& operator=(const CentralCache&) = delete;
  CentralCache(CentralCache&&) = delete;
  CentralCache& operator=(CentralCache&&) = delete;
};

/**
 * @brief Page heap for large allocations and fallback
 *
 * Handles allocations that are too large for the central cache
 * or when the central cache needs to be refilled.
 */
class PEREGRINE_API PageHeap {
 public:
  /**
   * @brief Constructor
   * @param underlying_allocator The underlying memory allocator to use
   */
  explicit PageHeap(IMemoryAllocatorPtr underlying_allocator);

  /**
   * @brief Allocate a large block of memory
   * @param size Size in bytes to allocate
   * @return Pointer to allocated memory, or nullptr on failure
   */
  void* Allocate(size_t size);

  /**
   * @brief Deallocate a large block of memory
   * @param ptr Pointer to deallocate
   * @param size Original allocation size (for statistics)
   */
  void Deallocate(void* ptr, size_t size);

  /**
   * @brief Allocate aligned memory
   * @param size Size in bytes to allocate
   * @param alignment Alignment requirement
   * @return Pointer to aligned memory, or nullptr on failure
   */
  void* AllocateAligned(size_t size, size_t alignment);

  /**
   * @brief Get page heap statistics
   */
  struct HeapStats {
    size_t total_allocated = 0;
    size_t active_allocations = 0;
    size_t peak_usage = 0;
    size_t allocation_count = 0;
    size_t deallocation_count = 0;
  };
  HeapStats GetStats() const;

 private:
  IMemoryAllocatorPtr underlying_allocator_;
  std::atomic<size_t> total_allocated_{0};
  std::atomic<size_t> active_allocations_{0};
  std::atomic<size_t> peak_usage_{0};
  std::atomic<size_t> allocation_count_{0};
  std::atomic<size_t> deallocation_count_{0};

  // Disable copy and move
  PageHeap(const PageHeap&) = delete;
  PageHeap& operator=(const PageHeap&) = delete;
  PageHeap(PageHeap&&) = delete;
  PageHeap& operator=(PageHeap&&) = delete;
};

}  // namespace amp
}  // namespace peregrine




