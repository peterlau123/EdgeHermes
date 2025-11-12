#include "NovaLLM/memory/allocator.h"

#include <cstdlib>

namespace nova_llm {


CPUAllocator::CPUAllocator() {}

CPUAllocator::~CPUAllocator() {}

void *CPUAllocator::do_allocate(size_t size) { return std::malloc(size); }

void CPUAllocator::do_deallocate(void *ptr) {
  if (ptr) {
    std::free(ptr);
  }
}


}  // namespace nova_llm