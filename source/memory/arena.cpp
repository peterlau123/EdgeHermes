#include "EdgeHermes/memory/arena.h"
#include "thread_cache_storage.h"

#include <algorithm>
#include <memory>

namespace edgehermes {
namespace amp {

// ArenaRouter Implementation
ArenaRouter::ArenaRouter(const AMPConfig& config) : config_(config) {
  // Initialize with empty arenas - they will be added via InitializeArenas
}

void ArenaRouter::InitializeArenas(IMemoryAllocatorPtr cpu_allocator,
                                  IMemoryAllocatorPtr gpu_allocator) {
  arenas_.clear();

  // Create CPU arena if CPU allocator provided
  if (cpu_allocator) {
    auto cpu_arena = std::make_unique<CPUArena>(config_, std::move(cpu_allocator), config_.numa_aware);
    arenas_.push_back(std::move(cpu_arena));
  }

  // Create GPU arena if GPU allocator provided
  if (gpu_allocator) {
    // TODO: GPU arena implementation is planned for future release
    // For now, we'll skip GPU arena creation and log the intent
    // auto gpu_arena = std::make_unique<GPUArena>(config_, std::move(gpu_allocator), false);
    // arenas_.push_back(std::move(gpu_arena));
  }
}

IArena* ArenaRouter::GetArena(DeviceType device_type) {
  auto it = std::find_if(arenas_.begin(), arenas_.end(),
                        [device_type](const std::unique_ptr<IArena>& arena) {
                          return arena->GetDeviceType() == device_type;
                        });
  return it != arenas_.end() ? it->get() : nullptr;
}

void* ArenaRouter::Allocate(size_t size, DeviceType device_type) {
  IArena* arena = GetArena(device_type);
  if (!arena) {
    return nullptr;
  }
  return arena->Allocate(size);
}

void ArenaRouter::Deallocate(void* ptr, size_t size, DeviceType device_type) {
  IArena* arena = GetArena(device_type);
  if (arena) {
    arena->Deallocate(ptr, size);
  }
}

MemoryStats ArenaRouter::GetGlobalStats() const {
  MemoryStats global_stats;
  for (const auto& arena : arenas_) {
    MemoryStats arena_stats = arena->GetStats();
    global_stats.total_allocated += arena_stats.total_allocated;
    global_stats.active_allocations += arena_stats.active_allocations;
    // Use worst fragmentation ratio
    global_stats.fragmentation_ratio = std::max(global_stats.fragmentation_ratio,
                                                arena_stats.fragmentation_ratio);
  }
  return global_stats;
}

bool ArenaRouter::AreAllArenasHealthy() const {
  return std::all_of(arenas_.begin(), arenas_.end(),
                    [](const std::unique_ptr<IArena>& arena) {
                      return arena->IsHealthy();
                    });
}

// CPUArena Implementation
CPUArena::CPUArena(const AMPConfig& config, IMemoryAllocatorPtr underlying_allocator, bool numa_aware)
    : config_(config),
      size_class_system_(edgehermes::amp::GetSizeClassSystem()),
      total_allocations_(0),
      total_deallocations_(0),
      active_allocations_(0),
      total_bytes_allocated_(0) {
  // Initialize thread cache storage if not already done
  edgehermes::amp::ThreadCacheStorage::Initialize(
      size_class_system_, config);

  // Create central cache
  central_cache_ = std::make_unique<CentralCache>(
      size_class_system_, config.central_cache_limit_mb);

  // Create page heap
  page_heap_ = std::make_unique<PageHeap>(std::move(underlying_allocator));
}

CPUArena::~CPUArena() {
  // Smart pointers handle cleanup
}

void* CPUArena::Allocate(size_t size) {
  if (size == 0) return nullptr;

  total_allocations_.fetch_add(1, std::memory_order_relaxed);

  // Try thread-local cache first for small allocations
  if (size_class_system_.IsSmallClass(size_class_system_.GetSizeClass(size))) {
    edgehermes::amp::ThreadCache& thread_cache = edgehermes::amp::ThreadCacheStorage::Get();
    void* ptr = thread_cache.Allocate(size_class_system_.GetSizeClass(size));
    if (ptr) {
      total_bytes_allocated_.fetch_add(size, std::memory_order_relaxed);
      active_allocations_.fetch_add(1, std::memory_order_relaxed);
      return ptr;
    }
  }

  // Fall back to central cache
  auto objects = central_cache_->AllocateBatch(size_class_system_.GetSizeClass(size), 1);
  if (!objects.empty()) {
    total_bytes_allocated_.fetch_add(size, std::memory_order_relaxed);
    active_allocations_.fetch_add(1, std::memory_order_relaxed);
    return objects[0];
  }

  // Last resort: page heap for large allocations
  void* ptr = page_heap_->Allocate(size);
  if (ptr) {
    total_bytes_allocated_.fetch_add(size, std::memory_order_relaxed);
    active_allocations_.fetch_add(1, std::memory_order_relaxed);
  }
  return ptr;
}

void CPUArena::Deallocate(void* ptr, size_t size) {
  if (!ptr || size == 0) return;

  total_deallocations_.fetch_add(1, std::memory_order_relaxed);
  active_allocations_.fetch_sub(1, std::memory_order_relaxed);

  // Determine size class
  size_t size_class = size_class_system_.GetSizeClass(size);

  // Try thread-local cache for small objects
  if (size_class_system_.IsSmallClass(size_class)) {
    edgehermes::amp::ThreadCache& thread_cache = edgehermes::amp::ThreadCacheStorage::Get();
    if (thread_cache.Deallocate(ptr, size_class)) {
      return;  // Successfully cached
    }
  }

  // Return to central cache
  central_cache_->DeallocateBatch(size_class, {ptr});
}

void* CPUArena::AllocateAligned(size_t size, size_t alignment) {
  // For aligned allocations, we use the page heap which handles alignment
  if (size == 0) return nullptr;

  total_allocations_.fetch_add(1, std::memory_order_relaxed);

  void* ptr = page_heap_->AllocateAligned(size, alignment);
  if (ptr) {
    total_bytes_allocated_.fetch_add(size, std::memory_order_relaxed);
    active_allocations_.fetch_add(1, std::memory_order_relaxed);
  }
  return ptr;
}

MemoryStats CPUArena::GetStats() const {
  MemoryStats stats;
  stats.total_allocated = total_bytes_allocated_.load(std::memory_order_relaxed);
  stats.active_allocations = active_allocations_.load(std::memory_order_relaxed);

  // Get central cache stats
  auto central_stats = central_cache_->GetStats();
  stats.total_allocated += central_stats.total_bytes;

  // Get page heap stats
  auto page_stats = page_heap_->GetStats();
  stats.total_allocated += page_stats.total_allocated;
  stats.active_allocations += page_stats.active_allocations;

  // Estimate fragmentation (simplified)
  if (stats.total_allocated > 0) {
    stats.fragmentation_ratio = 1.0 - (stats.active_allocations * 64.0 / stats.total_allocated);
    stats.fragmentation_ratio = std::max(0.0, std::min(1.0, stats.fragmentation_ratio));
  }

  return stats;
}

bool CPUArena::IsHealthy() const {
  // Basic health check - for const method, just check if components exist
  // A more thorough check would require non-const operations
  return central_cache_ && page_heap_;
}

// GPUArena Implementation (Stub)
GPUArena::GPUArena(const AMPConfig& config, IMemoryAllocatorPtr underlying_allocator, bool cuda_managed)
    : config_(config) {
  // TODO: GPU arena implementation is planned for future release
  // This is a placeholder that logs the intent but doesn't actually allocate

  // For now, we'll create a page heap but mark it as non-functional for GPU
  page_heap_ = std::make_unique<PageHeap>(std::move(underlying_allocator));
}

GPUArena::~GPUArena() {
  // Smart pointers handle cleanup
}



void* GPUArena::Allocate(size_t size) {
  // TODO: Implement GPU memory allocation
  // For now, return nullptr to indicate GPU allocation is not supported
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  return nullptr;
}

void GPUArena::Deallocate(void* ptr, size_t size) {
  // TODO: Implement GPU memory deallocation
  if (ptr) {
    total_deallocations_.fetch_add(1, std::memory_order_relaxed);
    active_allocations_.fetch_sub(1, std::memory_order_relaxed);
  }
}

void* GPUArena::AllocateAligned(size_t size, size_t alignment) {
  // TODO: Implement aligned GPU memory allocation
  total_allocations_.fetch_add(1, std::memory_order_relaxed);
  return nullptr;
}

MemoryStats GPUArena::GetStats() const {
  MemoryStats stats;
  stats.total_allocated = total_bytes_allocated_.load(std::memory_order_relaxed);
  stats.active_allocations = active_allocations_.load(std::memory_order_relaxed);
  stats.fragmentation_ratio = 0.0;  // Not implemented yet
  return stats;
}

bool GPUArena::IsHealthy() const {
  // GPU arena is not implemented yet, so report as unhealthy
  return false;
}

}  // namespace amp
}  // namespace edgehermes



