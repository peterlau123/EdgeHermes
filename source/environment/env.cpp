#pragma once

#include "NovaLLM/env.h"
#include "NovaLLM/memory/buffer_manager.h"

namespace nova_llm {

  void Env::setup(const Config& config) {

      BufferManager::Config bm_config;
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
      BufferManager::Builder::build(bm_config);

      // Additional setup based on other configuration options can be added here
  }

  void Env::teardown() {
      // Clean up resources and reset environment state
      // (Implementation details would go here)
  }

}  // namespace nova_llm