#pragma once

#include <memory>

#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/buffer_define.h"
#include "NovaLLM/memory/amp_system.h"
#include "NovaLLM/memory/arena.h"
#include "NovaLLM/memory/allocator_wrapper.h"

namespace nova_llm {

/**
 * @brief Adaptive Memory Pool (AMP) Buffer Manager
 *
 * Modern replacement for the legacy BufferManager using the AMP system.
 * Provides the same API but with superior performance and scalability.
 */
class NOVA_LLM_API AMPBufferManager {
 public:
  /**
   * @brief Configuration for AMP Buffer Manager
   */
  struct Config {
    nova_llm::amp::AMPConfig amp_config;

    // Legacy compatibility - device flags
    DeviceTypeFlags device_flags;

    // Allocator options for each device type
    std::unordered_map<DeviceType, nova_llm::amp::IMemoryAllocatorSharedPtr> allocators;
  };

  /**
   * @brief Builder for creating AMP Buffer Manager instances
   */
  class Builder {
   public:
    /**
     * @brief Build a new AMP Buffer Manager instance
     * @param config Configuration for the manager
     * @return Unique pointer to the created manager
     */
    static std::unique_ptr<AMPBufferManager> Build(const Config& config);

    /**
     * @brief Get the global AMP Buffer Manager instance
     * @return Reference to the global instance
     */
    static AMPBufferManager& GetInstance();
  };

  /**
   * @brief Constructor
   * @param config Configuration for the AMP system
   */
  explicit AMPBufferManager(Config config);

  // Disable copy and move
  AMPBufferManager(const AMPBufferManager&) = delete;
  AMPBufferManager& operator=(const AMPBufferManager&) = delete;
  AMPBufferManager(AMPBufferManager&&) = delete;
  AMPBufferManager& operator=(AMPBufferManager&&) = delete;

  /**
   * @brief Check if the manager is initialized
   * @return true if initialized and ready to use
   */
  [[nodiscard]] bool IsInitialized() const { return initialized_; }

  /**
   * @brief Fetch a buffer of the specified size and device type
   * @param size Size in bytes to allocate
   * @param device_type Target device type
   * @return Buffer structure containing allocated memory
   */
  Buffer Fetch(size_t size, DeviceType device_type);

  /**
   * @brief Return a buffer to the pool and clear it
   * @param buffer Buffer to return (will be cleared)
   */
  void Put(Buffer& buffer);

  /**
   * @brief Get memory statistics
   * @return Memory usage statistics
   */
  nova_llm::amp::MemoryStats GetStats() const;

  /**
   * @brief Check if all arenas are healthy
   * @return true if all device arenas are operating normally
   */
  bool IsHealthy() const;

  /**
   * @brief Get the underlying arena router (for advanced usage)
   * @return Pointer to the arena router
   */
  nova_llm::amp::ArenaRouter* GetArenaRouter() { return arena_router_.get(); }

  /**
   * @brief Destructor
   */
  ~AMPBufferManager();

 private:
  /**
   * @brief Initialize the AMP system
   * @param config Configuration
   * @return true on success
   */
  bool Initialize(const Config& config);

  // Member variables
  bool initialized_ = false;
  Config config_;
  std::unique_ptr<nova_llm::amp::ArenaRouter> arena_router_;

  // Global instance for singleton pattern
  static std::unique_ptr<AMPBufferManager> global_instance_;
};

}  // namespace nova_llm
