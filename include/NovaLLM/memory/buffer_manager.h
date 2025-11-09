#pragma once
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <memory>
#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/allocator.h"
#include "NovaLLM/memory/buffer_define.h"

namespace nova_llm {
/*
 * @todo: use segregated free list
 * */
class BufferHub;

class BufferManager {

 public:
  struct Config {
    DeviceTypeFlags device_flags;

    struct CPU {
      IAllocatorSharedPtr alloc {nullptr};
    };

    CPU cpu;

    struct GPU {
      IAllocatorSharedPtr alloc {nullptr};
    };

    GPU gpu;

    struct METAL {
      IAllocatorSharedPtr alloc {nullptr};
    };

    METAL metal;
  };

  class Builder {
   public:
    static BufferManager& build(const Config& config);
    static BufferManager& getInstance();

   private:
    static BufferManager buffer_manager;
  };

  BufferManager(const BufferManager&) = delete;  // Disable copy constructor

  BufferManager& operator=(const BufferManager&) = delete;  // Disable copy assignment

  BufferManager(BufferManager&&) = delete;  // Disable move constructor

  BufferManager& operator=(BufferManager&&) = delete;  // Disable move assignment

  [[nodiscard("Do not drop isInit return value")]] bool isInited() const { return is_init_; }

  Buffer fetch(size_t size, DeviceType device_type);

  void put(const Buffer& buffer);

  ~BufferManager();

 private:

  void destroy();

  BufferManager() = default;

  bool init(const Config& config);

  bool is_init_ {false};

  std::unordered_map<DeviceType, BufferHub*> buffer_hubs_;
};

}  // namespace nova_llm
