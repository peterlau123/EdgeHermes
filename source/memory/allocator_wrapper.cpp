#include "NovaLLM/memory/allocator_wrapper.h"

#include <cstdlib>
#include <new>
#include <vector>

#ifdef NOVA_LLM_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#include "NovaLLM/utils/log.h"

namespace nova_llm {
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
  // TODO: Configure TCMalloc with options
  // For now, just note that TCMalloc integration requires:
  // - libtcmalloc.so/libtcmalloc.dylib
  // - tc_malloc, tc_free, tc_memalign functions
}

void* TCMallocAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;
  // TODO: Use tc_malloc when TCMalloc is available
  // return tc_malloc(size);
  return std::malloc(size);  // Fallback to standard malloc
}

void TCMallocAllocator::Deallocate(void* ptr) {
  if (ptr) {
    // TODO: Use tc_free when TCMalloc is available
    // tc_free(ptr);
    std::free(ptr);  // Fallback to standard free
  }
}

void* TCMallocAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;
  // TODO: Use tc_memalign when TCMalloc is available
  // return tc_memalign(alignment, size);
  return AllocateAligned(size, alignment);  // Fallback
}



// Jemalloc Allocator Implementation
JemallocAllocator::JemallocAllocator(const std::unordered_map<std::string, std::string>& options) {
  // TODO: Configure jemalloc with options
  // For now, just note that jemalloc integration requires:
  // - libjemalloc.so/libjemalloc.dylib
  // - je_malloc, je_free, je_aligned_alloc functions
}

void* JemallocAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;
  // TODO: Use je_malloc when jemalloc is available
  // return je_malloc(size);
  return std::malloc(size);  // Fallback to standard malloc
}

void JemallocAllocator::Deallocate(void* ptr) {
  if (ptr) {
    // TODO: Use je_free when jemalloc is available
    // je_free(ptr);
    std::free(ptr);  // Fallback to standard free
  }
}

void* JemallocAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;
  // TODO: Use je_aligned_alloc when jemalloc is available
  // return je_aligned_alloc(alignment, size);
  return AllocateAligned(size, alignment);  // Fallback
}



// Mimalloc Allocator Implementation
MimallocAllocator::MimallocAllocator(const std::unordered_map<std::string, std::string>& options) {
  // TODO: Configure mimalloc with options
  // For now, just note that mimalloc integration requires:
  // - libmimalloc.so/libmimalloc.dylib
  // - mi_malloc, mi_free, mi_aligned_alloc functions
}

void* MimallocAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;
  // TODO: Use mi_malloc when mimalloc is available
  // return mi_malloc(size);
  return std::malloc(size);  // Fallback to standard malloc
}

void MimallocAllocator::Deallocate(void* ptr) {
  if (ptr) {
    // TODO: Use mi_free when mimalloc is available
    // mi_free(ptr);
    std::free(ptr);  // Fallback to standard free
  }
}

void* MimallocAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;
  // TODO: Use mi_aligned_alloc when mimalloc is available
  // return mi_aligned_alloc(alignment, size);
  return AllocateAligned(size, alignment);  // Fallback
}



// CUDA Allocator Implementation
CUDAAllocator::CUDAAllocator(bool use_managed_memory)
    : use_managed_memory_(use_managed_memory) {
  // Check CUDA availability at runtime
  cuda_available_ = CheckCudaAvailability();
  if (!cuda_available_) {
    LOG_WARN("CUDA not available, CUDAAllocator will fallback to standard allocation");
  }
}

bool CUDAAllocator::CheckCudaAvailability() {
#ifdef NOVA_LLM_ENABLE_CUDA
  // Check if CUDA runtime is available
  cudaError_t err = cudaGetDeviceCount(&device_count_);
  if (err != cudaSuccess) {
    LOG_DEBUG("CUDA not available: %s", cudaGetErrorString(err));
    return false;
  }

  if (device_count_ == 0) {
    LOG_DEBUG("No CUDA devices found");
    return false;
  }

  LOG_INFO("CUDA available with %d device(s)", device_count_);
  return true;
#else
  return false;
#endif
}

void* CUDAAllocator::Allocate(size_t size) {
  if (size == 0) return nullptr;

#ifdef NOVA_LLM_ENABLE_CUDA
  if (cuda_available_) {
    void* ptr = nullptr;
    cudaError_t err;

    if (use_managed_memory_) {
      // Use CUDA managed memory (accessible from both CPU and GPU)
      err = cudaMallocManaged(&ptr, size);
      if (err == cudaSuccess) {
        LOG_DEBUG("Allocated %zu bytes of CUDA managed memory at %p", size, ptr);
        return ptr;
      } else {
        LOG_ERROR("CUDA managed memory allocation failed: %s", cudaGetErrorString(err));
      }
    } else {
      // Use regular CUDA device memory
      err = cudaMalloc(&ptr, size);
      if (err == cudaSuccess) {
        LOG_DEBUG("Allocated %zu bytes of CUDA device memory at %p", size, ptr);
        return ptr;
      } else {
        LOG_ERROR("CUDA device memory allocation failed: %s", cudaGetErrorString(err));
      }
    }
  }
#endif

  // Fallback to standard allocation
  LOG_DEBUG("CUDA not available, falling back to standard allocation for %zu bytes", size);
  return std::malloc(size);
}

void CUDAAllocator::Deallocate(void* ptr) {
  if (!ptr) return;

#ifdef NOVA_LLM_ENABLE_CUDA
  if (cuda_available_) {
    // Try to determine if this is CUDA memory
    // For managed memory, cudaFree will work
    // For device memory, cudaFree is required
    cudaError_t err = cudaFree(ptr);
    if (err == cudaSuccess) {
      LOG_DEBUG("Freed CUDA memory at %p", ptr);
      return;
    } else {
      LOG_DEBUG("cudaFree failed for %p: %s, trying standard free", ptr, cudaGetErrorString(err));
    }
  }
#endif

  // Fallback to standard deallocation
  std::free(ptr);
}

void* CUDAAllocator::AllocateAligned(size_t size, size_t alignment) {
  if (size == 0) return nullptr;

#ifdef NOVA_LLM_ENABLE_CUDA
  if (cuda_available_) {
    // CUDA has specific alignment requirements
    // For CUDA managed memory, alignment should be at least 256 bytes
    // For simplicity, we'll use CUDA's managed allocation which handles alignment
    if (use_managed_memory_ && alignment <= 256) {
      return Allocate(size);  // CUDA managed memory handles alignment
    }

    // For regular CUDA memory or larger alignment requirements,
    // we need to handle alignment manually
    // CUDA doesn't provide aligned allocation directly, so we allocate extra and align

    // Calculate total size needed (original + alignment + alignment overhead)
    size_t total_size = size + alignment;

    void* raw_ptr = nullptr;
    cudaError_t err;

    if (use_managed_memory_) {
      err = cudaMallocManaged(&raw_ptr, total_size);
    } else {
      err = cudaMalloc(&raw_ptr, total_size);
    }

    if (err != cudaSuccess) {
      LOG_ERROR("CUDA aligned allocation failed: %s", cudaGetErrorString(err));
      return nullptr;
    }

    // Align the pointer
    uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw_ptr);
    uintptr_t aligned_addr = (raw_addr + alignment - 1) & ~(alignment - 1);
    void* aligned_ptr = reinterpret_cast<void*>(aligned_addr);

    // Store the original pointer before the aligned pointer for deallocation
    void** original_ptr_location = reinterpret_cast<void**>(aligned_ptr) - 1;
    *original_ptr_location = raw_ptr;

    LOG_DEBUG("Allocated %zu bytes of aligned CUDA memory (alignment %zu) at %p (raw: %p)",
              size, alignment, aligned_ptr, raw_ptr);
    return aligned_ptr;
  }
#endif

  // Fallback to standard aligned allocation
  return AllocateAligned(size, alignment);
}



// AllocatorFactory Implementation
IMemoryAllocatorPtr AllocatorFactory::Create(AllocatorType type,
                                           const std::unordered_map<std::string, std::string>& options) {
  switch (type) {
    case AllocatorType::STANDARD:
      return std::make_unique<StandardAllocator>();
    case AllocatorType::TCMALLOC:
      return std::make_unique<TCMallocAllocator>(options);
    case AllocatorType::JEMALLOC:
      return std::make_unique<JemallocAllocator>(options);
    case AllocatorType::MIMALLOC:
      return std::make_unique<MimallocAllocator>(options);
    default:
      return std::make_unique<StandardAllocator>();
  }
}

bool AllocatorFactory::IsAvailable(AllocatorType type) {
  switch (type) {
    case AllocatorType::STANDARD:
      return true;
    case AllocatorType::TCMALLOC:
      // TODO: Check if TCMalloc library is available
      return false;
    case AllocatorType::JEMALLOC:
      // TODO: Check if jemalloc library is available
      return false;
    case AllocatorType::MIMALLOC:
      // TODO: Check if mimalloc library is available
      return false;
    default:
      return false;
  }
}

std::vector<AllocatorType> AllocatorFactory::GetAvailableAllocators() {
  std::vector<AllocatorType> available;
  available.push_back(AllocatorType::STANDARD);
  // TODO: Check and add other allocators if available
  return available;
}

const char* AllocatorFactory::GetAllocatorName(AllocatorType type) {
  switch (type) {
    case AllocatorType::STANDARD:
      return "Standard";
    case AllocatorType::TCMALLOC:
      return "TCMalloc";
    case AllocatorType::JEMALLOC:
      return "Jemalloc";
    case AllocatorType::MIMALLOC:
      return "Mimalloc";
    default:
      return "Unknown";
  }
}

}  // namespace amp
}  // namespace nova_llm
