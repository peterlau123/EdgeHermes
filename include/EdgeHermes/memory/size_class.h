#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <array>

#include "EdgeHermes/utils/macros.h"

namespace edgehermes {
namespace amp {

/**
 * @brief Adaptive size class system for efficient memory allocation
 *
 * Maps allocation sizes to efficient size classes based on usage patterns.
 * Uses a hybrid approach with fixed classes for small sizes and dynamic
 * optimization for larger sizes.
 */
class edgehermes_API SizeClassSystem {
 public:
  // Constants
  static constexpr size_t NUM_SIZE_CLASSES = 128;
  static constexpr size_t MAX_SMALL_SIZE = 64 * 1024;  // 64KB

  /**
   * @brief Default constructor
   */
  SizeClassSystem();

  /**
   * @brief Get the size class for a given allocation size
   * @param size Allocation size in bytes
   * @return Size class ID (0 to NUM_SIZE_CLASSES-1)
   */
  [[nodiscard]] size_t GetSizeClass(size_t size) const;

  /**
   * @brief Get the maximum allocation size for a size class
   * @param class_id Size class ID
   * @return Maximum size that fits in this class
   */
  [[nodiscard]] size_t GetClassMaxSize(size_t class_id) const;

  /**
   * @brief Get the minimum allocation size for a size class
   * @param class_id Size class ID
   * @return Minimum size that fits in this class
   */
  [[nodiscard]] size_t GetClassMinSize(size_t class_id) const;

  /**
   * @brief Check if a size class is for small objects (fits in thread cache)
   * @param class_id Size class ID
   * @return true if class is for small objects
   */
  [[nodiscard]] bool IsSmallClass(size_t class_id) const;

  /**
   * @brief Get the page size multiplier for a size class
   * @param class_id Size class ID
   * @return Number of pages needed for batch allocation
   */
  [[nodiscard]] size_t GetPageMultiplier(size_t class_id) const;

  /**
   * @brief Update size class usage statistics for adaptive optimization
   * @param class_id Size class ID
   * @param allocation_size Actual allocation size
   */
  void UpdateUsageStats(size_t class_id, size_t allocation_size);

 private:
  /**
   * @brief Initialize size class boundaries
   * Uses geometric progression for small sizes, then linear for larger sizes
   */
  void InitializeSizeClasses();

  /**
   * @brief Size class boundaries (max size for each class)
   */
  std::array<size_t, NUM_SIZE_CLASSES> size_class_max_;

  /**
   * @brief Size class minimum sizes (for reference)
   */
  std::array<size_t, NUM_SIZE_CLASSES> size_class_min_;

  /**
   * @brief Page multipliers for batch allocation
   */
  std::array<size_t, NUM_SIZE_CLASSES> page_multipliers_;

  /**
   * @brief Usage statistics for adaptive optimization
   */
  struct ClassStats {
    size_t allocation_count = 0;
    size_t total_allocated_bytes = 0;
    double average_size = 0.0;
  };
  std::array<ClassStats, NUM_SIZE_CLASSES> stats_;
};

// Global size class system instance
extern edgehermes_API const SizeClassSystem& GetSizeClassSystem();

}  // namespace amp
}  // namespace edgehermes



