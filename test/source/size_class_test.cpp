#include "NovaLLM/memory/size_class.h"

#include <gtest/gtest.h>
#include <unordered_set>

using namespace nova_llm::amp;

class SizeClassTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  const SizeClassSystem& size_class_system = GetSizeClassSystem();
};

// Test basic size class functionality
TEST_F(SizeClassTest, GetSizeClassBasic) {
  // Test small sizes
  EXPECT_EQ(size_class_system.GetSizeClass(8), 0);
  EXPECT_EQ(size_class_system.GetSizeClass(16), 1);
  EXPECT_EQ(size_class_system.GetSizeClass(32), 2);
  EXPECT_EQ(size_class_system.GetSizeClass(64), 3);

  // Test medium sizes
  EXPECT_EQ(size_class_system.GetSizeClass(128), 4);
  EXPECT_EQ(size_class_system.GetSizeClass(256), 5);

  // Test large sizes
  EXPECT_EQ(size_class_system.GetSizeClass(1024), size_class_system.GetSizeClass(2048));
}

TEST_F(SizeClassTest, GetSizeClassBoundaries) {
  // Test that sizes at boundaries map to correct classes
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES - 1; ++class_id) {
    size_t max_size = size_class_system.GetClassMaxSize(class_id);
    size_t next_min_size = size_class_system.GetClassMinSize(class_id + 1);

    // Max of this class should be less than min of next class
    EXPECT_LT(max_size, next_min_size);

    // Size at boundary should map to correct class
    EXPECT_EQ(size_class_system.GetSizeClass(max_size), class_id);
    EXPECT_EQ(size_class_system.GetSizeClass(max_size + 1), class_id + 1);
  }
}

TEST_F(SizeClassTest, GetClassMaxSize) {
  // Test that max sizes are monotonically increasing
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES - 1; ++class_id) {
    size_t current_max = size_class_system.GetClassMaxSize(class_id);
    size_t next_max = size_class_system.GetClassMaxSize(class_id + 1);
    EXPECT_LE(current_max, next_max);
  }
}

TEST_F(SizeClassTest, GetClassMinSize) {
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    size_t min_size = size_class_system.GetClassMinSize(class_id);
    size_t max_size = size_class_system.GetClassMaxSize(class_id);

    EXPECT_LE(min_size, max_size);

    if (class_id > 0) {
      size_t prev_max = size_class_system.GetClassMaxSize(class_id - 1);
      EXPECT_EQ(min_size, prev_max + 1);
    }
  }
}

TEST_F(SizeClassTest, IsSmallClass) {
  // First few classes should be small
  EXPECT_TRUE(size_class_system.IsSmallClass(0));
  EXPECT_TRUE(size_class_system.IsSmallClass(1));
  EXPECT_TRUE(size_class_system.IsSmallClass(2));

  // Later classes should not be small
  size_t last_small_class = SizeClassSystem::NUM_SIZE_CLASSES - 1;
  for (; last_small_class > 0; --last_small_class) {
    if (size_class_system.GetClassMaxSize(last_small_class) <= SizeClassSystem::MAX_SMALL_SIZE) {
      EXPECT_TRUE(size_class_system.IsSmallClass(last_small_class));
      break;
    }
  }

  // Classes larger than MAX_SMALL_SIZE should not be small
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    if (size_class_system.GetClassMaxSize(class_id) > SizeClassSystem::MAX_SMALL_SIZE) {
      EXPECT_FALSE(size_class_system.IsSmallClass(class_id));
    }
  }
}

TEST_F(SizeClassTest, GetPageMultiplier) {
  // Test that page multipliers are reasonable
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    size_t multiplier = size_class_system.GetPageMultiplier(class_id);
    EXPECT_GE(multiplier, 1);
    EXPECT_LE(multiplier, 8);  // Reasonable upper bound
  }
}

TEST_F(SizeClassTest, SizeClassCoverage) {
  // Test that all reasonable sizes are covered
  std::unordered_set<size_t> covered_classes;

  // Test powers of 2
  for (size_t size = 1; size <= 1024 * 1024; size *= 2) {
    size_t class_id = size_class_system.GetSizeClass(size);
    EXPECT_LT(class_id, SizeClassSystem::NUM_SIZE_CLASSES);
    covered_classes.insert(class_id);
  }

  // Test some intermediate sizes
  std::vector<size_t> test_sizes = {1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383};
  for (size_t size : test_sizes) {
    size_t class_id = size_class_system.GetSizeClass(size);
    EXPECT_LT(class_id, SizeClassSystem::NUM_SIZE_CLASSES);
    covered_classes.insert(class_id);
  }

  // Should have covered multiple classes
  EXPECT_GT(covered_classes.size(), 5);
}

TEST_F(SizeClassTest, StatisticsUpdate) {
  // Test that statistics can be updated
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    size_t test_size = size_class_system.GetClassMinSize(class_id);

    // This should not crash
    const_cast<SizeClassSystem&>(size_class_system).UpdateUsageStats(class_id, test_size);
  }
}

TEST_F(SizeClassTest, BoundaryConditions) {
  // Test edge cases
  EXPECT_EQ(size_class_system.GetSizeClass(0), 0);  // Size 0 should map to first class
  EXPECT_EQ(size_class_system.GetSizeClass(1), 0);  // Size 1 should map to first class

  // Very large sizes should map to last class
  EXPECT_EQ(size_class_system.GetSizeClass(std::numeric_limits<size_t>::max()),
            SizeClassSystem::NUM_SIZE_CLASSES - 1);
}

TEST_F(SizeClassTest, ClassSizeRanges) {
  // Verify that each class has a reasonable size range
  for (size_t class_id = 0; class_id < SizeClassSystem::NUM_SIZE_CLASSES; ++class_id) {
    size_t min_size = size_class_system.GetClassMinSize(class_id);
    size_t max_size = size_class_system.GetClassMaxSize(class_id);

    EXPECT_LE(min_size, max_size);
    EXPECT_GT(max_size, 0);

    // All sizes in this range should map to this class
    for (size_t size = min_size; size <= std::min(max_size, min_size + 100); ++size) {
      EXPECT_EQ(size_class_system.GetSizeClass(size), class_id);
    }
  }
}

TEST_F(SizeClassTest, GlobalInstance) {
  // Test that the global instance is accessible
  const SizeClassSystem& global1 = GetSizeClassSystem();
  const SizeClassSystem& global2 = GetSizeClassSystem();

  // Should be the same instance
  EXPECT_EQ(&global1, &global2);

  // Should have valid data
  EXPECT_EQ(global1.GetSizeClass(64), global2.GetSizeClass(64));
}

TEST_F(SizeClassTest, SizeClassDistribution) {
  // Test that sizes are distributed across classes reasonably
  std::vector<size_t> class_counts(SizeClassSystem::NUM_SIZE_CLASSES, 0);

  // Sample many sizes and count class usage
  for (size_t size = 1; size <= 10000; ++size) {
    size_t class_id = size_class_system.GetSizeClass(size);
    if (class_id < class_counts.size()) {
      class_counts[class_id]++;
    }
  }

  // Should have used multiple classes
  int used_classes = 0;
  for (size_t count : class_counts) {
    if (count > 0) {
      used_classes++;
    }
  }

  EXPECT_GT(used_classes, 3);  // Should use at least a few classes
}

TEST_F(SizeClassTest, LargeSizeHandling) {
  // Test that very large sizes are handled correctly
  const size_t very_large_size = 1024 * 1024 * 1024;  // 1GB
  size_t class_id = size_class_system.GetSizeClass(very_large_size);

  EXPECT_LT(class_id, SizeClassSystem::NUM_SIZE_CLASSES);

  // Should be one of the larger classes
  EXPECT_GE(class_id, SizeClassSystem::NUM_SIZE_CLASSES / 2);
}
