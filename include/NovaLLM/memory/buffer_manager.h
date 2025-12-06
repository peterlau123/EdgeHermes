#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/allocator.h"
#include "NovaLLM/memory/buffer_define.h"
#include "NovaLLM/memory/amp_buffer_manager.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

// Feature flag for AMP system - now enabled by default
#ifndef USE_AMP_BUFFER_MANAGER
#define USE_AMP_BUFFER_MANAGER 1
#endif

namespace nova_llm {

/*
 * Legacy BufferManager API - now implemented using AMP (Adaptive Memory Pool) system
 * This provides backwards compatibility while using the new high-performance memory management.
 */
class NOVA_LLM_API BufferManager {
 public:
  struct Config {
    DeviceTypeFlags device_flags;

    struct CPU {
      IAllocatorSharedPtr alloc{nullptr};
    };

    CPU cpu;

    struct GPU {
      IAllocatorSharedPtr alloc{nullptr};
    };

    GPU gpu;

    struct METAL {
      IAllocatorSharedPtr alloc{nullptr};
    };

    METAL metal;
  };

  class Builder {
   public:
    NOVA_LLM_API static BufferManager& build(const Config& config);
    NOVA_LLM_API static BufferManager& getInstance();
  };

  // Legacy API - now delegates to AMP system
  // Note: Constructor is public for Builder access, but class is still non-copyable
  NOVA_LLM_API BufferManager();
  NOVA_LLM_API BufferManager(const BufferManager&) = delete;
  NOVA_LLM_API BufferManager& operator=(const BufferManager&) = delete;
  NOVA_LLM_API BufferManager(BufferManager&&) = delete;
  NOVA_LLM_API BufferManager& operator=(BufferManager&&) = delete;

  NOVA_LLM_API bool isInited() const;

  NOVA_LLM_API Buffer fetch(size_t size, DeviceType device_type);

  // Return a buffer obtained from fetch back to the pool and clear it.
  NOVA_LLM_API void put(Buffer& buffer);

  NOVA_LLM_API ~BufferManager();

  NOVA_LLM_API void destroy();

 private:
  bool init(const Config& config);

  // Internal AMP system - using direct composition for simplicity
  std::unique_ptr<AMPBufferManager> amp_manager_;
};

}  // namespace nova_llm

#ifdef _MSC_VER
#pragma warning(pop)
#endif
