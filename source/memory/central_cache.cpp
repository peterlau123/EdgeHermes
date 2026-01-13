#include "EdgeHermes/memory/central_cache.h"
#include "EdgeHermes/memory/amp_system.h"
#include "EdgeHermes/memory/allocator.h"

#include <algorithm>
#include <memory>

namespace edgehermes {
namespace amp {

CentralCache::CentralCache(const SizeClassSystem& size_class_system, size_t max_cache_size_mb)
    : size_class_system_(size_class_system), max_cache_size_mb_(max_cache_size_mb) {
}

CentralCache::~CentralCache() {
  // Return all cached objects to page heap
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    ReturnToPageHeap(class_id);
  }
}

std::vector<void*> CentralCache::AllocateBatch(size_t size_class, size_t count) {
  if (size_class >= SizeClassSystem::NUM_SIZE_CLASSES) {
    return {};
  }

  auto& list = size_class_lists_[size_class];
  std::lock_guard<std::mutex> lock(list.mutex);

  std::vector<void*> result;

  // Take objects from the existing list
  size_t available = std::min(count, list.objects.size());
  result.reserve(available);

  for (size_t i = 0; i < available; ++i) {
    result.push_back(list.objects.back());
    list.objects.pop_back();
  }

  // Update cache size
  size_t object_size = size_class_system_.GetClassMaxSize(size_class);
  list.total_bytes -= available * object_size;
  current_cache_size_mb_.fetch_sub((available * object_size) / (1024 * 1024),
                                  std::memory_order_relaxed);

  // If we didn't get enough, try to refill from page heap
  size_t remaining = count - available;
  if (remaining > 0 && !IsAtCapacity()) {
    size_t refilled = RefillFromPageHeap(size_class, remaining);
    if (refilled > 0) {
      // Take additional objects from the newly refilled list
      size_t additional = std::min(remaining, refilled);
      for (size_t i = 0; i < additional; ++i) {
        result.push_back(list.objects.back());
        list.objects.pop_back();
      }

      // Update cache size again
      list.total_bytes -= additional * object_size;
      current_cache_size_mb_.fetch_sub((additional * object_size) / (1024 * 1024),
                                      std::memory_order_relaxed);
    }
  }

  return result;
}

void CentralCache::DeallocateBatch(size_t size_class, const std::vector<void*>& objects) {
  if (size_class >= SizeClassSystem::NUM_SIZE_CLASSES || objects.empty()) {
    return;
  }

  auto& list = size_class_lists_[size_class];
  std::lock_guard<std::mutex> lock(list.mutex);

  // Check if we should accept these objects
  size_t object_size = size_class_system_.GetClassMaxSize(size_class);
  size_t new_bytes = objects.size() * object_size;
  size_t new_cache_mb = (list.total_bytes + new_bytes) / (1024 * 1024);

  if (new_cache_mb >= max_cache_size_mb_) {
    // Cache is too full, return objects directly to page heap
    // This is a placeholder - in real implementation would call page heap
    return;
  }

  // Add objects to cache
  list.objects.insert(list.objects.end(), objects.begin(), objects.end());
  list.total_bytes += new_bytes;
  current_cache_size_mb_.fetch_add(new_cache_mb, std::memory_order_relaxed);
}

CentralCache::CacheStats CentralCache::GetStats() const {
  CacheStats stats;
  stats.cache_limit_mb = max_cache_size_mb_;

  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    const auto& list = size_class_lists_[class_id];
    std::lock_guard<std::mutex> lock(list.mutex);

    stats.objects_per_class[class_id] = list.objects.size();
    stats.total_objects += list.objects.size();
    stats.total_bytes += list.total_bytes;
  }

  return stats;
}

bool CentralCache::IsAtCapacity() const {
  return current_cache_size_mb_.load(std::memory_order_relaxed) >= max_cache_size_mb_;
}

size_t CentralCache::RefillFromPageHeap(size_t size_class, size_t count) {
  // This is a placeholder implementation
  // In a real system, this would allocate from the PageHeap
  // For now, return 0 to indicate no allocation
  return 0;
}

void CentralCache::ReturnToPageHeap(size_t size_class) {
  auto& list = size_class_lists_[size_class];
  std::lock_guard<std::mutex> lock(list.mutex);

  if (!list.objects.empty()) {
    // This is a placeholder - in real implementation would return to page heap
    // For now, just clear the cache
    list.objects.clear();
    current_cache_size_mb_.fetch_sub(list.total_bytes / (1024 * 1024),
                                    std::memory_order_relaxed);
    list.total_bytes = 0;
  }
}

// PageHeap implementation

PageHeap::PageHeap(IMemoryAllocatorPtr underlying_allocator)
    : underlying_allocator_(std::move(underlying_allocator)) {
  if (!underlying_allocator_) {
    throw std::invalid_argument("PageHeap requires a valid underlying allocator");
  }
}

void* PageHeap::Allocate(size_t size) {
  void* ptr = underlying_allocator_->Allocate(size);
  if (ptr) {
    allocation_count_.fetch_add(1, std::memory_order_relaxed);
    active_allocations_.fetch_add(1, std::memory_order_relaxed);
    total_allocated_.fetch_add(size, std::memory_order_relaxed);

    size_t current_total = total_allocated_.load(std::memory_order_relaxed);
    size_t current_peak = peak_usage_.load(std::memory_order_relaxed);
    while (current_total > current_peak &&
           !peak_usage_.compare_exchange_weak(current_peak, current_total)) {
      // Retry if peak was updated by another thread
    }
  }
  return ptr;
}

void PageHeap::Deallocate(void* ptr, size_t size) {
  if (ptr) {
    underlying_allocator_->Deallocate(ptr);
    deallocation_count_.fetch_add(1, std::memory_order_relaxed);
    active_allocations_.fetch_sub(1, std::memory_order_relaxed);
    total_allocated_.fetch_sub(size, std::memory_order_relaxed);
  }
}

void* PageHeap::AllocateAligned(size_t size, size_t alignment) {
  void* ptr = underlying_allocator_->AllocateAligned(size, alignment);
  if (ptr) {
    allocation_count_.fetch_add(1, std::memory_order_relaxed);
    active_allocations_.fetch_add(1, std::memory_order_relaxed);
    total_allocated_.fetch_add(size, std::memory_order_relaxed);

    size_t current_total = total_allocated_.load(std::memory_order_relaxed);
    size_t current_peak = peak_usage_.load(std::memory_order_relaxed);
    while (current_total > current_peak &&
           !peak_usage_.compare_exchange_weak(current_peak, current_total)) {
      // Retry if peak was updated by another thread
    }
  }
  return ptr;
}

PageHeap::HeapStats PageHeap::GetStats() const {
  HeapStats stats;
  stats.total_allocated = total_allocated_.load(std::memory_order_relaxed);
  stats.active_allocations = active_allocations_.load(std::memory_order_relaxed);
  stats.peak_usage = peak_usage_.load(std::memory_order_relaxed);
  stats.allocation_count = allocation_count_.load(std::memory_order_relaxed);
  stats.deallocation_count = deallocation_count_.load(std::memory_order_relaxed);
  return stats;
}

}  // namespace amp
}  // namespace edgehermes



