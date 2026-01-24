#include "Peregrine/memory/allocator.h"

#include <memory>

namespace peregrine {
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
#ifdef edgehermes_ENABLE_TCMALLOC
      return true;
#else
      return false;
#endif
    case AllocatorType::JEMALLOC:
#ifdef edgehermes_ENABLE_JEMALLOC
      return true;
#else
      return false;
#endif
    case AllocatorType::MIMALLOC:
#ifdef edgehermes_ENABLE_MIMALLOC
      return true;
#else
      return false;
#endif
    default:
      return false;
  }
}

std::vector<AllocatorType> AllocatorFactory::GetAvailableAllocators() {
  std::vector<AllocatorType> available;
  available.push_back(AllocatorType::STANDARD);

#ifdef edgehermes_ENABLE_TCMALLOC
  available.push_back(AllocatorType::TCMALLOC);
#endif

#ifdef edgehermes_ENABLE_JEMALLOC
  available.push_back(AllocatorType::JEMALLOC);
#endif

#ifdef edgehermes_ENABLE_MIMALLOC
  available.push_back(AllocatorType::MIMALLOC);
#endif

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
}  // namespace peregrine</content>




