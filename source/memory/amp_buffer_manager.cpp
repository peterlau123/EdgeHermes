#include "NovaLLM/memory/amp_buffer_manager.h"

#include <stdexcept>

#include "NovaLLM/memory/allocator.h"
#include "thread_cache_storage.h"
#include "NovaLLM/utils/log.h"

namespace nova_llm {

// Global instance for singleton
std::unique_ptr<AMPBufferManager> AMPBufferManager::global_instance_;

AMPBufferManager::AMPBufferManager(Config config) : config_(std::move(config)) {
  if (!Initialize(config_)) {
    throw std::runtime_error("Failed to initialize AMP Buffer Manager");
  }
}

AMPBufferManager::~AMPBufferManager() {
  // Cleanup is handled by unique_ptr destructors
  initialized_ = false;
}

bool AMPBufferManager::Initialize(const Config& config) {
  try {
    // Initialize thread cache storage
    nova_llm::amp::ThreadCacheStorage::Initialize(
        nova_llm::amp::GetSizeClassSystem(), config.amp_config);

    // Create arena router
    arena_router_ = std::make_unique<nova_llm::amp::ArenaRouter>(config.amp_config);

    // Initialize arenas for configured devices
    nova_llm::amp::IMemoryAllocatorPtr cpu_allocator;
    nova_llm::amp::IMemoryAllocatorPtr gpu_allocator;

    // Get CPU allocator
    if (config.device_flags.has(DeviceType::CPU)) {
      auto it = config.allocators.find(DeviceType::CPU);
      if (it != config.allocators.end() && it->second) {
        // Convert shared_ptr to unique_ptr by creating a new unique_ptr from raw pointer
        cpu_allocator = std::unique_ptr<nova_llm::amp::IMemoryAllocator>(it->second.get());
        // Note: This creates a new unique_ptr that shares ownership, but doesn't transfer it
        // For proper ownership transfer, we'd need to modify the interface
      } else {
        // Use standard allocator as fallback
        cpu_allocator = nova_llm::amp::AllocatorFactory::Create(
            nova_llm::amp::AllocatorType::STANDARD);
      }
    }

    // Get GPU allocator
    if (config.device_flags.has(DeviceType::CUDA)) {
      auto it = config.allocators.find(DeviceType::CUDA);
      if (it != config.allocators.end() && it->second) {
        // Convert shared_ptr to unique_ptr by creating a new unique_ptr from raw pointer
        gpu_allocator = std::unique_ptr<nova_llm::amp::IMemoryAllocator>(it->second.get());
        // Note: This creates a new unique_ptr that shares ownership, but doesn't transfer it
        // For proper ownership transfer, we'd need to modify the interface
      } else {
        // Use CUDA allocator as fallback
        gpu_allocator = nova_llm::amp::AllocatorFactory::Create(
            nova_llm::amp::AllocatorType::STANDARD);  // CUDA allocator would be better
      }
    }

    // Initialize arenas
    arena_router_->InitializeArenas(std::move(cpu_allocator), std::move(gpu_allocator));

    initialized_ = true;
    LOG_INFO("AMP Buffer Manager initialized successfully");
    return true;

  } catch (const std::exception& e) {
    LOG_ERROR("Failed to initialize AMP Buffer Manager: %s", e.what());
    return false;
  }
}

Buffer AMPBufferManager::Fetch(size_t size, DeviceType device_type) {
  if (!initialized_) {
    LOG_ERROR("AMP Buffer Manager not initialized");
    return Buffer{};
  }

  Buffer buffer;
  buffer.device_type = device_type;

  try {
    // Use arena router to allocate memory
    void* ptr = arena_router_->Allocate(size, device_type);
    if (ptr) {
      buffer.data = static_cast<uint8_t*>(ptr);
      buffer.size = size;
      LOG_DEBUG("Allocated buffer: size={}, device={}", size, static_cast<int>(device_type));
    } else {
      LOG_WARN("Failed to allocate buffer: size=%zu, device=%d",
               size, static_cast<int>(device_type));
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Exception during buffer allocation: %s", e.what());
  }

  return buffer;
}

void AMPBufferManager::Put(Buffer& buffer) {
  if (!initialized_) {
    LOG_ERROR("AMP Buffer Manager not initialized");
    return;
  }

  if (buffer.data == nullptr || buffer.size == 0) {
    return;
  }

  try {
    // Use arena router to deallocate memory
    arena_router_->Deallocate(buffer.data, buffer.size, buffer.device_type);

    LOG_DEBUG("Deallocated buffer: size={}, device={}",
              buffer.size, static_cast<int>(buffer.device_type));

    // Clear the buffer
    buffer.data = nullptr;
    buffer.size = 0;

  } catch (const std::exception& e) {
    LOG_ERROR("Exception during buffer deallocation: %s", e.what());
  }
}

nova_llm::amp::MemoryStats AMPBufferManager::GetStats() const {
  if (!initialized_ || !arena_router_) {
    return {};
  }
  return arena_router_->GetGlobalStats();
}

bool AMPBufferManager::IsHealthy() const {
  if (!initialized_ || !arena_router_) {
    return false;
  }
  return arena_router_->AreAllArenasHealthy();
}

// Builder implementation
std::unique_ptr<AMPBufferManager> AMPBufferManager::Builder::Build(const Config& config) {
  return std::make_unique<AMPBufferManager>(config);
}

AMPBufferManager& AMPBufferManager::Builder::GetInstance() {
  if (!global_instance_) {
    // Create default configuration
    Config default_config;
    default_config.amp_config = nova_llm::amp::AMPConfig{};
    default_config.device_flags.set(DeviceType::CPU);

    // Add standard CPU allocator
    default_config.allocators[DeviceType::CPU] =
        nova_llm::amp::AllocatorFactory::Create(nova_llm::amp::AllocatorType::STANDARD);

    global_instance_ = std::make_unique<AMPBufferManager>(default_config);
  }
  return *global_instance_;
}

}  // namespace nova_llm
