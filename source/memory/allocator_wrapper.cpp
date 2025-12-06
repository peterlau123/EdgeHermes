#include "NovaLLM/memory/allocator.h"

#include <memory>

namespace nova_llm {
namespace amp {

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
}  // namespace nova_llm</content>
