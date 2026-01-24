#include "Peregrine/memory/size_class.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace peregrine {
namespace amp {

SizeClassSystem::SizeClassSystem() {
  InitializeSizeClasses();
}

size_t SizeClassSystem::GetSizeClass(size_t size) const {
  // Binary search for the appropriate size class
  auto it = std::lower_bound(size_class_max_.begin(), size_class_max_.end(), size);
  if (it == size_class_max_.end()) {
    // Size too large, return last class
    return NUM_SIZE_CLASSES - 1;
  }
  return std::distance(size_class_max_.begin(), it);
}

size_t SizeClassSystem::GetClassMaxSize(size_t class_id) const {
  if (class_id >= NUM_SIZE_CLASSES) {
    return 0;
  }
  return size_class_max_[class_id];
}

size_t SizeClassSystem::GetClassMinSize(size_t class_id) const {
  if (class_id >= NUM_SIZE_CLASSES) {
    return 0;
  }
  return size_class_min_[class_id];
}

bool SizeClassSystem::IsSmallClass(size_t class_id) const {
  if (class_id >= NUM_SIZE_CLASSES) {
    return false;
  }
  return size_class_max_[class_id] <= MAX_SMALL_SIZE;
}

size_t SizeClassSystem::GetPageMultiplier(size_t class_id) const {
  if (class_id >= NUM_SIZE_CLASSES) {
    return 1;
  }
  return page_multipliers_[class_id];
}

void SizeClassSystem::UpdateUsageStats(size_t class_id, size_t allocation_size) {
  if (class_id >= NUM_SIZE_CLASSES) {
    return;
  }

  auto& stat = stats_[class_id];
  stat.allocation_count++;
  stat.total_allocated_bytes += allocation_size;

  // Update running average
  if (stat.allocation_count == 1) {
    stat.average_size = static_cast<double>(allocation_size);
  } else {
    double alpha = 0.1;  // Exponential moving average factor
    stat.average_size = alpha * allocation_size + (1.0 - alpha) * stat.average_size;
  }
}

void SizeClassSystem::InitializeSizeClasses() {
  // Initialize size class boundaries using a hybrid approach:
  // - Small sizes: geometric progression (64B to 64KB)
  // - Large sizes: linear progression with larger steps

  // Small size classes (geometric progression)
  size_t current_size = 64;  // Start at 64 bytes
  size_t class_id = 0;

  // First 64 classes: geometric progression
  while (class_id < 64 && current_size <= MAX_SMALL_SIZE) {
    size_class_min_[class_id] = (class_id == 0) ? 1 : size_class_max_[class_id - 1] + 1;
    size_class_max_[class_id] = current_size;
    page_multipliers_[class_id] = 1;  // Small objects don't need batching

    current_size = static_cast<size_t>(current_size * 1.25);  // 25% growth
    class_id++;
  }

  // Medium size classes (64KB to 1MB)
  current_size = 64 * 1024;  // 64KB
  size_t step = 16 * 1024;   // 16KB steps

  while (class_id < 96 && current_size <= 1024 * 1024) {
    size_class_min_[class_id] = size_class_max_[class_id - 1] + 1;
    size_class_max_[class_id] = current_size;
    page_multipliers_[class_id] = 2;  // Batch allocate 2 pages

    current_size += step;
    step *= 2;  // Double the step size
    class_id++;
  }

  // Large size classes (1MB+)
  current_size = 2 * 1024 * 1024;  // 2MB
  step = 1024 * 1024;  // 1MB steps

  while (class_id < NUM_SIZE_CLASSES) {
    size_class_min_[class_id] = size_class_max_[class_id - 1] + 1;
    size_class_max_[class_id] = current_size;
    page_multipliers_[class_id] = 4;  // Batch allocate 4 pages

    current_size += step;
    class_id++;
  }

  // Ensure the last class covers very large allocations
  if (class_id > 0) {
    size_class_max_[NUM_SIZE_CLASSES - 1] = std::numeric_limits<size_t>::max();
  }
}

// Global instance
static SizeClassSystem global_size_class_system;

const SizeClassSystem& GetSizeClassSystem() {
  return global_size_class_system;
}

}  // namespace amp
}  // namespace peregrine




