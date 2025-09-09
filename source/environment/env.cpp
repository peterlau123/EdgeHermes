#include "NovaLLM/env.h"

#include "NovaLLM/memory/buffer_manager.h"

namespace nova_llm {

void Env::init(const Config& config) {

  BufferManager::Config bm_config;
  if (config.enable_custom) {
    bm_config.device_flags = config.device_flags;
    // Initialize CPU allocator if provided
    if (config.device_flags.has(DeviceType::CPU) && config.cpu_allocator) {
      // Set up CPU environment with the provided allocator
      // (Implementation details would go here)
      bm_config.cpu.alloc = config.cpu_allocator;
    }

    // Initialize GPU allocator if provided
    if (config.device_flags.has(DeviceType::CUDA) && config.gpu_allocator) {
      // Set up GPU environment with the provided allocator
      // (Implementation details would go here)
      bm_config.gpu.alloc = config.gpu_allocator;
    }

    // Additional setup based on other configuration options can be added here
  } else {
    bm_config.device_flags.set(DeviceType::CPU);
    bm_config.device_flags.set(DeviceType::CUDA);
    // Default CPU allocator
    bm_config.cpu.alloc = std::make_shared<CPUAllocator>();
    // Default GPU allocator
    bm_config.gpu.alloc = std::make_shared<CUDAAllocator>();
  }
  BufferManager::Builder::build(bm_config);
}

void Env::deinit() {
  // Clean up resources and reset environment state
  // (Implementation details would go here)
  BufferManager::Builder::getInstance().~BufferManager();
}

}  // namespace nova_llm