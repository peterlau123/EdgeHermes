#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "EdgeHermes/utils/macros.h"
#include "EdgeHermes/memory/amp_system.h"

namespace edgehermes {
namespace amp {

/**
 * @brief Standard allocator wrapper using std::malloc/free
 *
 * Provides the baseline allocator implementation using standard C library functions.
 */
class edgehermes_API StandardAllocator : public IMemoryAllocator {
 public:
  StandardAllocator() = default;

  void* Allocate(size_t size) override;
  void Deallocate(void* ptr) override;
  void* AllocateAligned(size_t size, size_t alignment) override;

  const char* Name() const override { return "Standard"; }
};

/**
 * @brief TCMalloc wrapper
 *
 * Integrates Google TCMalloc for high-performance CPU memory allocation.
 * TCMalloc provides excellent performance for multi-threaded applications.
 */
class edgehermes_API TCMallocAllocator : public IMemoryAllocator {
 public:
  /**
   * @brief Constructor
   * @param options Configuration options for TCMalloc
   */
  explicit TCMallocAllocator(const std::unordered_map<std::string, std::string>& options = {});

  void* Allocate(size_t size) override;
  void Deallocate(void* ptr) override;
  void* AllocateAligned(size_t size, size_t alignment) override;

  const char* Name() const override { return "TCMalloc"; }

 private:
  // TCMalloc-specific configuration would be stored here
};

/**
 * @brief Jemalloc wrapper
 *
 * Integrates Facebook jemalloc for high-performance memory allocation.
 * Jemalloc is known for its excellent fragmentation control and performance.
 */
class edgehermes_API JemallocAllocator : public IMemoryAllocator {
 public:
  /**
   * @brief Constructor
   * @param options Configuration options for jemalloc
   */
  explicit JemallocAllocator(const std::unordered_map<std::string, std::string>& options = {});

  void* Allocate(size_t size) override;
  void Deallocate(void* ptr) override;
  void* AllocateAligned(size_t size, size_t alignment) override;

  const char* Name() const override { return "Jemalloc"; }

 private:
  // Jemalloc-specific configuration would be stored here
};

/**
 * @brief Mimalloc wrapper
 *
 * Integrates Microsoft mimalloc for modern, high-performance memory allocation.
 * Mimalloc is designed for modern systems and provides excellent performance.
 */
class edgehermes_API MimallocAllocator : public IMemoryAllocator {
 public:
  /**
   * @brief Constructor
   * @param options Configuration options for mimalloc
   */
  explicit MimallocAllocator(const std::unordered_map<std::string, std::string>& options = {});

  void* Allocate(size_t size) override;
  void Deallocate(void* ptr) override;
  void* AllocateAligned(size_t size, size_t alignment) override;

  const char* Name() const override { return "Mimalloc"; }

 private:
  // Mimalloc-specific configuration would be stored here
};

/**
 * @brief GPU allocator wrapper (CUDA)
 *
 * Handles CUDA memory allocation with support for managed memory.
 */
class edgehermes_API CUDAAllocator : public IMemoryAllocator {
 public:
  /**
   * @brief Constructor
   * @param use_managed_memory Whether to use CUDA managed memory
   */
  explicit CUDAAllocator(bool use_managed_memory = false);

  void* Allocate(size_t size) override;
  void Deallocate(void* ptr) override;
  void* AllocateAligned(size_t size, size_t alignment) override;

  const char* Name() const override { return "CUDA"; }

 private:
  /**
   * @brief Check if CUDA is available on this system
   * @return true if CUDA is available and functional
   */
  bool CheckCudaAvailability();

  bool use_managed_memory_;
  bool cuda_available_;
  int device_count_;
};

/**
 * @brief Factory for creating allocator instances
 *
 * Provides a centralized way to create and configure memory allocators
 * based on type and options.
 */
class edgehermes_API AllocatorFactory {
 public:
  /**
   * @brief Create an allocator instance
   * @param type Allocator type to create
   * @param options Configuration options for the allocator
   * @return Unique pointer to the created allocator
   */
  static IMemoryAllocatorPtr Create(AllocatorType type,
                                   const std::unordered_map<std::string, std::string>& options = {});

  /**
   * @brief Check if an allocator type is available
   * @param type Allocator type to check
   * @return true if the allocator is available on this system
   */
  static bool IsAvailable(AllocatorType type);

  /**
   * @brief Get available allocator types on this system
   * @return List of available allocator types
   */
  static std::vector<AllocatorType> GetAvailableAllocators();

  /**
   * @brief Get allocator name as string
   * @param type Allocator type
   * @return String representation of the allocator type
   */
  static const char* GetAllocatorName(AllocatorType type);
};

}  // namespace amp
}  // namespace edgehermes



