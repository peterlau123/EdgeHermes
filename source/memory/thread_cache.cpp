#include "Peregrine/memory/thread_cache.h"
#include "thread_cache_storage.h"
#include "Peregrine/memory/amp_system.h"

#include <algorithm>
#include <memory>
#include <new>

namespace peregrine {
namespace amp {

// Thread-local storage implementation
thread_local std::unique_ptr<ThreadCache> ThreadCacheStorage::cache_;
const SizeClassSystem* ThreadCacheStorage::size_class_system_ = nullptr;
AMPConfig ThreadCacheStorage::config_;

ThreadCache::ThreadCache(const SizeClassSystem& size_class_system, size_t max_cache_size_kb)
    : size_class_system_(size_class_system), max_cache_size_kb_(max_cache_size_kb) {
  // Initialize free lists
  for (auto& list : free_lists_) {
    list.head.store(nullptr);
    list.length.store(0);
  }
}

ThreadCache::~ThreadCache() {
  // Flush all cached objects back to central cache
  Flush();
}

void* ThreadCache::Allocate(size_t size_class) {
  if (size_class >= MAX_SIZE_CLASSES) {
    return nullptr;
  }

  // Try to allocate from thread cache first
  void* ptr = PopFreeList(free_lists_[size_class]);
  if (ptr != nullptr) {
    cache_hits_.fetch_add(1, std::memory_order_relaxed);
    return ptr;
  }

  // Cache miss - allocate from central cache
  cache_misses_.fetch_add(1, std::memory_order_relaxed);

  // Try batch allocation to refill cache
  const size_t batch_size = std::min(size_t(32), MAX_OBJECTS_PER_CLASS / 4);
  auto batch = BatchAllocate(size_class, batch_size);

  if (!batch.empty()) {
    // Cache all but one object
    for (size_t i = 1; i < batch.size(); ++i) {
      PushFreeList(free_lists_[size_class], static_cast<FreeListNode*>(batch[i]));
    }
    return batch[0];
  }

  // Fallback to direct allocation from central cache
  return nullptr;
}

bool ThreadCache::Deallocate(void* ptr, size_t size_class) {
  if (size_class >= MAX_SIZE_CLASSES || ptr == nullptr) {
    return false;
  }

  // Check if cache is full
  if (IsFull(size_class)) {
    return false;  // Send to central cache
  }

  // Cache the object
  PushFreeList(free_lists_[size_class], static_cast<FreeListNode*>(ptr));
  return true;
}

void ThreadCache::Flush() {
  // Flush all cached objects to central cache
  for (size_t class_id = 0; class_id < MAX_SIZE_CLASSES; ++class_id) {
    std::vector<void*> objects;
    objects.reserve(MAX_OBJECTS_PER_CLASS);

    // Collect all objects from this size class
    while (auto node = PopFreeList(free_lists_[class_id])) {
      objects.push_back(node);
    }

    if (!objects.empty()) {
      BatchDeallocate(class_id, objects);
    }
  }
}

ThreadCache::CacheStats ThreadCache::GetStats() const {
  CacheStats stats;
  stats.hits = cache_hits_.load(std::memory_order_relaxed);
  stats.misses = cache_misses_.load(std::memory_order_relaxed);

  // Count total cached objects
  for (const auto& list : free_lists_) {
    stats.total_objects += list.length.load(std::memory_order_relaxed);
  }

  // Estimate bytes (rough approximation)
  stats.total_bytes = stats.total_objects * 64;  // Assume average 64 bytes per object

  return stats;
}

bool ThreadCache::IsFull(size_t size_class) const {
  if (size_class >= MAX_SIZE_CLASSES) {
    return true;
  }

  return free_lists_[size_class].length.load(std::memory_order_relaxed) >= MAX_OBJECTS_PER_CLASS;
}

void ThreadCache::PushFreeList(FreeList& list, FreeListNode* node) {
  if (!node) return;

  size_t current_length = list.length.load(std::memory_order_relaxed);
  if (current_length >= MAX_OBJECTS_PER_CLASS) {
    return;  // Cache is full
  }

  FreeListNode* old_head = list.head.load(std::memory_order_relaxed);
  do {
    node->next = old_head;
  } while (!list.head.compare_exchange_weak(old_head, node, std::memory_order_release));

  list.length.fetch_add(1, std::memory_order_relaxed);
}

ThreadCache::FreeListNode* ThreadCache::PopFreeList(FreeList& list) {
  FreeListNode* old_head = list.head.load(std::memory_order_relaxed);
  FreeListNode* new_head;

  do {
    if (old_head == nullptr) {
      return nullptr;
    }
    new_head = old_head->next;
  } while (!list.head.compare_exchange_weak(old_head, new_head, std::memory_order_acquire));

  list.length.fetch_sub(1, std::memory_order_relaxed);
  return old_head;
}

std::vector<void*> ThreadCache::BatchAllocate(size_t size_class, size_t count) {
  // This is a placeholder - in a real implementation, this would
  // coordinate with the CentralCache to allocate batches
  // For now, return empty vector to indicate no batch allocation
  return {};
}

void ThreadCache::BatchDeallocate(size_t size_class, const std::vector<void*>& objects) {
  // This is a placeholder - in a real implementation, this would
  // coordinate with the CentralCache to deallocate batches
  // For now, do nothing
}

// ThreadCacheStorage implementation

ThreadCache& ThreadCacheStorage::Get() {
  if (!cache_) {
    if (!size_class_system_) {
      throw std::runtime_error("ThreadCacheStorage not initialized");
    }
    cache_ = std::make_unique<ThreadCache>(*size_class_system_, config_.thread_cache_size_kb);
  }
  return *cache_;
}

void ThreadCacheStorage::Initialize(const SizeClassSystem& size_class_system,
                                   const AMPConfig& config) {
  size_class_system_ = &size_class_system;
  config_ = config;
}

void ThreadCacheStorage::Cleanup() {
  cache_.reset();
  size_class_system_ = nullptr;
}

}  // namespace amp
}  // namespace peregrine




