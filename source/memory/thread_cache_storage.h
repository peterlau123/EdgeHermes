#pragma once

#include <memory>

#include "EdgeHermes/memory/size_class.h"
#include "EdgeHermes/memory/amp_system.h"

namespace edgehermes {
namespace amp {

class ThreadCache;

/**
 * @brief Thread-local storage for thread caches
 */
class ThreadCacheStorage {
 public:
  /**
   * @brief Get thread-local cache instance
   * @return Reference to thread's cache
   */
  static ThreadCache& Get();

  /**
   * @brief Initialize thread cache storage
   * @param size_class_system Size class system reference
   * @param config AMP configuration
   */
  static void Initialize(const SizeClassSystem& size_class_system,
                         const AMPConfig& config);

  /**
   * @brief Cleanup thread cache storage
   */
  static void Cleanup();

 private:
  static thread_local std::unique_ptr<ThreadCache> cache_;
  static const SizeClassSystem* size_class_system_;
  static AMPConfig config_;
};

}  // namespace amp
}  // namespace edgehermes



