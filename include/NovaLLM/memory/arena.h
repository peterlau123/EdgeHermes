#pragma once

#include <memory>
#include <vector>

#include "NovaLLM/utils/macros.h"
#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/amp_system.h"
#include "NovaLLM/memory/size_class.h"
#include "NovaLLM/memory/central_cache.h"

namespace nova_llm {
namespace amp {

/**
 * @brief Base arena interface for device-specific memory management
 *
 * Arenas handle memory allocation for specific devices (CPU, GPU, etc.)
 * and provide device-aware optimizations like NUMA for CPU and CUDA-aware
 * for GPU allocations.
 */
class NOVA_LLM_API IArena {
 public:
  virtual ~IArena() = default;

  /**
   * @brief Get the device type this arena manages
   * @return Device type
   */
  virtual DeviceType GetDeviceType() const = 0;

  /**
   * @brief Allocate memory
   * @param size Size in bytes to allocate
   * @return Pointer to allocated memory, or nullptr on failure
   */
  virtual void* Allocate(size_t size) = 0;

  /**
   * @brief Deallocate memory
   * @param ptr Pointer to deallocate
   * @param size Original allocation size (for statistics)
   */
  virtual void Deallocate(void* ptr, size_t size) = 0;

  /**
   * @brief Allocate aligned memory
   * @param size Size in bytes to allocate
   * @param alignment Alignment requirement
   * @return Pointer to aligned memory, or nullptr on failure
   */
  virtual void* AllocateAligned(size_t size, size_t alignment) = 0;

  /**
   * @brief Get arena statistics
   */
  virtual MemoryStats GetStats() const = 0;

  /**
   * @brief Check if arena is healthy
   * @return true if arena is operating normally
   */
  virtual bool IsHealthy() const = 0;
};

/**
 * @brief CPU arena with NUMA-aware allocation
 *
 * Uses the AMP system optimized for CPU memory management
 * with thread-local caches and NUMA awareness.
 */
class NOVA_LLM_API CPUArena : public IArena {
 public:
  /**
   * @brief Constructor
   * @param config AMP configuration
   * @param underlying_allocator The underlying allocator to use
   * @param numa_aware Whether to use NUMA-aware allocation
   */
  CPUArena(const AMPConfig& config, IMemoryAllocatorPtr underlying_allocator, bool numa_aware = false);

  ~CPUArena() override;

  DeviceType GetDeviceType() const override { return DeviceType::CPU; }

  void* Allocate(size_t size) override;

  void Deallocate(void* ptr, size_t size) override;

  void* AllocateAligned(size_t size, size_t alignment) override;

  MemoryStats GetStats() const override;

  bool IsHealthy() const override;

 private:
  const AMPConfig& config_;
  const SizeClassSystem& size_class_system_;
  std::unique_ptr<CentralCache> central_cache_;
  std::unique_ptr<PageHeap> page_heap_;

  // Statistics
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> active_allocations_{0};
  std::atomic<size_t> total_bytes_allocated_{0};
};

/**
 * @brief GPU arena with CUDA-aware allocation
 *
 * Handles GPU memory allocation with CUDA-aware optimizations
 * and managed memory support.
 */
class NOVA_LLM_API GPUArena : public IArena {
 public:
  /**
   * @brief Constructor
   * @param config AMP configuration
   * @param underlying_allocator The underlying allocator to use
   * @param cuda_managed Whether to use CUDA managed memory
   */
  GPUArena(const AMPConfig& config, IMemoryAllocatorPtr underlying_allocator, bool cuda_managed = false);

  ~GPUArena() override;

  DeviceType GetDeviceType() const override { return DeviceType::CUDA; }

  void* Allocate(size_t size) override;

  void Deallocate(void* ptr, size_t size) override;

  void* AllocateAligned(size_t size, size_t alignment) override;

  MemoryStats GetStats() const override;

  bool IsHealthy() const override;

 private:
  const AMPConfig& config_;
  std::unique_ptr<PageHeap> page_heap_;  // GPU uses direct page heap allocation

  // Statistics
  std::atomic<size_t> total_allocations_{0};
  std::atomic<size_t> total_deallocations_{0};
  std::atomic<size_t> active_allocations_{0};
  std::atomic<size_t> total_bytes_allocated_{0};
};

/**
 * @brief Arena router for managing multiple device arenas
 *
 * Routes allocation requests to the appropriate device arena
 * and manages arena lifecycle.
 */
class NOVA_LLM_API ArenaRouter {
 public:
  /**
   * @brief Constructor
   * @param config AMP configuration
   */
  explicit ArenaRouter(const AMPConfig& config);

  /**
   * @brief Initialize arenas for all configured devices
   * @param cpu_allocator CPU allocator
   * @param gpu_allocator GPU allocator (optional)
   */
  void InitializeArenas(IMemoryAllocatorPtr cpu_allocator,
                       IMemoryAllocatorPtr gpu_allocator = nullptr);

  /**
   * @brief Get arena for specific device
   * @param device_type Device type
   * @return Pointer to arena, or nullptr if not available
   */
  IArena* GetArena(DeviceType device_type);

  /**
   * @brief Allocate memory on specific device
   * @param size Size in bytes
   * @param device_type Target device
   * @return Pointer to allocated memory
   */
  void* Allocate(size_t size, DeviceType device_type);

  /**
   * @brief Deallocate memory from specific device
   * @param ptr Pointer to deallocate
   * @param size Original size
   * @param device_type Device type
   */
  void Deallocate(void* ptr, size_t size, DeviceType device_type);

  /**
   * @brief Get statistics for all arenas
   * @return Memory statistics
   */
  MemoryStats GetGlobalStats() const;

  /**
   * @brief Check if all arenas are healthy
   * @return true if all arenas are operating normally
   */
  bool AreAllArenasHealthy() const;

 private:
  const AMPConfig& config_;
  std::vector<std::unique_ptr<IArena>> arenas_;
};

}  // namespace amp
}  // namespace nova_llm
