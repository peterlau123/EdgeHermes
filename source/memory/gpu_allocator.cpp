#include "../backend/gpu/nvidia_header.h"
#include "NovaLLM/memory/allocator.h"

namespace nova_llm {

#if ENABLE_CUDA

GPUAllocator::GPUAllocator() {}

GPUAllocator::~GPUAllocator() {}

void *GPUAllocator::do_allocate(size_t size) {
  void *ptr = nullptr;
  if (cudaMalloc(&ptr, size) != cudaSuccess) {
    return nullptr;
  }
  return ptr;
}

void GPUAllocator::do_deallocate(void *ptr) { cudaFree(ptr); }
#else

GPUAllocator::GPUAllocator() {}

GPUAllocator::~GPUAllocator() {}

void* GPUAllocator::do_allocate(size_t size) {
  // TODO: log error
  return nullptr;
}

void GPUAllocator::do_deallocate(void* ptr) {
  // do nothing
}


#endif
}  // namespace nova_llm