#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "NovaLLM/utils/macros.h"
#include "NovaLLM/memory/size_class.h"
#include "NovaLLM/memory/amp_system.h"

namespace nova_llm {
namespace amp {

/**
 * @brief Lock-free thread-local cache for small allocations
 *
 * Provides fast, per-thread allocation for small objects using atomic operations
 * to avoid synchronization overhead. Falls back to central cache for misses.
 */
class NOVA_LLM_API ThreadCache {
 public:
  // Constants
  static constexpr size_t MAX_SIZE_CLASSES = SizeClassSystem::NUM_SIZE_CLASSES;
  static constexpr size_t MAX_OBJECTS_PER_CLASS = 256;  // Max cached objects per size class

  /**
   * @brief Constructor
   * @param size_class_system Reference to the global size class system
   * @param max_cache_size_kb Maximum cache size in KB per thread
   */
  explicit ThreadCache(const SizeClassSystem& size_class_system,
                       size_t max_cache_size_kb = 512);

  /**
   * @brief Destructor - returns all cached objects to central cache
   */
  ~ThreadCache();

  /**
   * @brief Allocate memory from thread cache
   * @param size_class Size class ID
   * @return Pointer to allocated memory, or nullptr if cache miss
   */
  void* Allocate(size_t size_class);

  /**
   * @brief Deallocate memory to thread cache
   * @param ptr Pointer to deallocate
   * @param size_class Size class ID
   * @return true if cached, false if should go to central cache
   */
  bool Deallocate(void* ptr, size_t size_class);

  /**
   * @brief Flush cache to central cache (used during thread cleanup)
   */
  void Flush();

  /**
   * @brief Get cache statistics
   * @return Current cache statistics
   */
  struct CacheStats {
    size_t total_objects = 0;
    size_t total_bytes = 0;
    size_t hits = 0;
    size_t misses = 0;
  };
  CacheStats GetStats() const;

  /**
   * @brief Check if cache is full for a size class
   * @param size_class Size class ID
   * @return true if cache is at capacity
   */
  bool IsFull(size_t size_class) const;

 private:
  /**
   * @brief Node structure for lock-free linked list
   */
  struct FreeListNode {
    FreeListNode* next = nullptr;
  };

  /**
   * @brief Free list for each size class
   */
  struct FreeList {
    std::atomic<FreeListNode*> head{nullptr};
    std::atomic<size_t> length{0};
  };

  /**
   * @brief Push object to free list (lock-free)
   * @param list Target free list
   * @param node Node to push
   */
  void PushFreeList(FreeList& list, FreeListNode* node);

  /**
   * @brief Pop object from free list (lock-free)
   * @param list Source free list
   * @return Popped node, or nullptr if empty
   */
  FreeListNode* PopFreeList(FreeList& list);

  /**
   * @brief Batch allocate from central cache
   * @param size_class Size class ID
   * @param count Number of objects to allocate
   * @return Vector of allocated objects
   */
  std::vector<void*> BatchAllocate(size_t size_class, size_t count);

  /**
   * @brief Batch deallocate to central cache
   * @param size_class Size class ID
   * @param objects Objects to deallocate
   */
  void BatchDeallocate(size_t size_class, const std::vector<void*>& objects);

  // Member variables
  const SizeClassSystem& size_class_system_;
  std::array<FreeList, MAX_SIZE_CLASSES> free_lists_;
  size_t max_cache_size_kb_;
  std::atomic<size_t> current_cache_size_kb_{0};

  // Statistics
  std::atomic<size_t> cache_hits_{0};
  std::atomic<size_t> cache_misses_{0};

  // Disable copy and move
  ThreadCache(const ThreadCache&) = delete;
  ThreadCache& operator=(const ThreadCache&) = delete;
  ThreadCache(ThreadCache&&) = delete;
  ThreadCache& operator=(ThreadCache&&) = delete;
};

/**
 * @brief Thread-local storage for thread caches
 */
class NOVA_LLM_API ThreadCacheStorage {
 public:
  /**
   * @brief Get thread-local cache instance
   * @return Reference to thread's cache
   */
  static ThreadCache& Get();

  /**
   * @brief Initialize thread cache storage
   * @param size_class_system Size class system reference
   * @param config AMP configuration
   */
  static void Initialize(const SizeClassSystem& size_class_system,
                         const AMPConfig& config);

  /**
   * @brief Cleanup thread cache storage
   */
  static void Cleanup();

 private:
  static thread_local std::unique_ptr<ThreadCache> cache_;
  static const SizeClassSystem* size_class_system_;
  static AMPConfig config_;
};

}  // namespace amp
}  // namespace nova_llm
