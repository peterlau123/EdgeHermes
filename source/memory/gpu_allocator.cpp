#include "NovaLLM/memory/allocator.h"

#if defined(NOVA_LLM_CUDA_ON) && NOVA_LLM_CUDA_ON
namespace nova_llm {

CUDAAllocator::CUDAAllocator() = default;

CUDAAllocator::~CUDAAllocator() = default;

void* CUDAAllocator::do_allocate(size_t size) {
  void* ptr = nullptr;
  cudaError_t err = cudaMalloc(&ptr, size);
  if (err != cudaSuccess) {
    return nullptr;
  }
  return ptr;
}

void CUDAAllocator::do_deallocate(void* ptr) {
  cudaFree(ptr);
}

}
#endif