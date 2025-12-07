#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

#include "NovaLLM/common/device.h"
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

    // Note: Legacy allocator fields removed as AMP system now handles allocation internally
    // Custom allocators can be configured through AMP system if needed in the future
  };

  class Builder {
   public:
    NOVA_LLM_API static BufferManager& build(const Config& config);
    NOVA_LLM_API static BufferManager& getInstance();
  };

  // Legacy API - now delegates to AMP system
  // Note: Constructor is public for Builder access, but class is still non-copyable
  BufferManager();
  BufferManager(const BufferManager&) = delete;
  BufferManager& operator=(const BufferManager&) = delete;
  BufferManager(BufferManager&&) = delete;
  BufferManager& operator=(BufferManager&&) = delete;

  bool isInited() const;

  Buffer fetch(size_t size, DeviceType device_type);

  // Return a buffer obtained from fetch back to the pool and clear it.
  void put(Buffer& buffer);

  ~BufferManager();

  void destroy();

 private:
  bool init(const Config& config);

  // Internal AMP system - using direct composition for simplicity
  std::unique_ptr<AMPBufferManager> amp_manager_;
};

}  // namespace nova_llm

#ifdef _MSC_VER
#pragma warning(pop)
#endif
