#pragma once

#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/allocator.h"

namespace nova_llm {

class NOVA_LLM_API Env {

 public:
  struct Config {
    /*
    System will use default when false;User must set the following when true
    when false,device_flags will be DeviceType::CPU|DeviceType::CUDA
    cpu_allocator will be CPUAllocator
    gpu_allocator will be CudaAllocator
    */
    bool enable_custom {false};

    // Configuration options for the environment
    DeviceTypeFlags device_flags;

    IAllocatorSharedPtr cpu_allocator {nullptr};

    IAllocatorSharedPtr gpu_allocator {nullptr};
    // Add more configuration options as needed
  };

  static void init(const Config& config);

  static void deinit();
};

}  // namespace nova_llm