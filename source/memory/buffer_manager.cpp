#include "Peregrine/memory/buffer_manager.h"

#include <stdexcept>

#include "Peregrine/memory/amp_buffer_manager.h"
#include "Peregrine/memory/allocator.h"
#include "Peregrine/utils/log.h"

// Global instance for singleton pattern
static std::unique_ptr<peregrine::BufferManager> global_buffer_manager_;

peregrine::BufferManager::BufferManager() = default;

peregrine::BufferManager& peregrine::BufferManager::Builder::build(const Config& config) {
  if (!global_buffer_manager_) {
    global_buffer_manager_ = std::make_unique<BufferManager>();
    if (!global_buffer_manager_->init(config)) {
      throw std::runtime_error("Failed to initialize BufferManager");
    }
  }
  return *global_buffer_manager_;
}

peregrine::BufferManager& peregrine::BufferManager::Builder::getInstance() {
  if (!global_buffer_manager_) {
    // Create with default configuration
    Config default_config;
    default_config.device_flags.set(DeviceType::CPU);

    global_buffer_manager_ = std::make_unique<BufferManager>();
    if (!global_buffer_manager_->init(default_config)) {
      throw std::runtime_error("Failed to initialize BufferManager with default config");
    }
  }
  return *global_buffer_manager_;
}

peregrine::BufferManager::~BufferManager() = default;

bool peregrine::BufferManager::init(const Config& config) {
  if (amp_manager_) {
    return true; // Already initialized
  }

  try {
    // Convert legacy config to AMP config
    AMPBufferManager::Config amp_config;
    amp_config.amp_config = peregrine::amp::AMPConfig{};
    amp_config.device_flags = config.device_flags;

    // Set up allocators based on legacy config
    // Note: For now, we always use StandardAllocator since legacy IAllocator
    // interface is not directly compatible with IMemoryAllocator.
    // TODO: Create an adapter wrapper if custom allocators need to be supported
    if (config.device_flags.has(DeviceType::CPU)) {
      amp_config.allocators[DeviceType::CPU] =
          std::make_shared<peregrine::amp::StandardAllocator>();
    }

    if (config.device_flags.has(DeviceType::CUDA)) {
      // For GPU, use CUDA allocator (even though it's currently stubbed)
      // This ensures proper interface even if CUDA isn't available yet
      amp_config.allocators[DeviceType::CUDA] =
          std::make_shared<peregrine::amp::CUDAAllocator>(false);  // false = regular CUDA memory
    }

    // Create AMP buffer manager
    amp_manager_ = std::make_unique<AMPBufferManager>(std::move(amp_config));

    LOG_INFO("BufferManager initialized with AMP system");
    return true;

  } catch (const std::exception& e) {
    LOG_ERROR("Failed to initialize BufferManager with AMP system: %s", e.what());
    return false;
  }
}

bool peregrine::BufferManager::isInited() const {
  return amp_manager_ && amp_manager_->IsInitialized();
}

peregrine::Buffer peregrine::BufferManager::fetch(size_t size, DeviceType device_type) {
  if (!amp_manager_) {
    LOG_ERROR("BufferManager not initialized");
    return Buffer{};
  }
  return amp_manager_->Fetch(size, device_type);
}

void peregrine::BufferManager::put(Buffer& buffer) {
  if (!amp_manager_) {
    LOG_ERROR("BufferManager not initialized");
    return;
  }
  amp_manager_->Put(buffer);
}

void peregrine::BufferManager::destroy() {
  global_buffer_manager_.reset();
}




