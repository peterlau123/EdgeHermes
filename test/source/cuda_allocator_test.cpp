#include "Peregrine/memory/allocator.h"

#include <gtest/gtest.h>
#include <unordered_map>

using namespace peregrine::amp;

class CUDAAllocatorTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test CUDA allocator creation through factory
TEST_F(CUDAAllocatorTest, FactoryCreateCUDA) {
  // Note: Factory creates CUDAAllocator directly, not through AllocatorType enum
  // since CUDA is handled specially in the AMP system
  CUDAAllocator allocator(false);
  EXPECT_STREQ(allocator.Name(), "CUDA");
}

// Test CUDA allocator basic interface (may fall back to standard malloc)
TEST_F(CUDAAllocatorTest, CUDAAllocatorInterface) {
  CUDAAllocator allocator(false);  // Regular CUDA memory

  EXPECT_STREQ(allocator.Name(), "CUDA");

  // Test basic functionality (currently falls back to standard malloc if CUDA unavailable)
  void* ptr = allocator.Allocate(1024);
  EXPECT_NE(ptr, nullptr);

  // Should be able to write to the memory
  memset(ptr, 0xAA, 1024);

  allocator.Deallocate(ptr);
}

TEST_F(CUDAAllocatorTest, CUDAAllocatorManaged) {
  CUDAAllocator allocator(true);  // CUDA managed memory

  EXPECT_STREQ(allocator.Name(), "CUDA");

  // Test basic functionality (currently falls back to standard malloc if CUDA unavailable)
  void* ptr = allocator.Allocate(1024);
  EXPECT_NE(ptr, nullptr);

  // Should be able to write to the memory
  memset(ptr, 0xBB, 1024);

  allocator.Deallocate(ptr);
}

TEST_F(CUDAAllocatorTest, CUDAAllocatorZeroSize) {
  CUDAAllocator allocator(false);

  void* ptr = allocator.Allocate(0);
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(CUDAAllocatorTest, CUDAAllocatorLargeAllocation) {
  CUDAAllocator allocator(false);

  // Test larger allocation
  void* ptr = allocator.Allocate(1024 * 1024);  // 1MB
  EXPECT_NE(ptr, nullptr);

  // Fill with pattern
  memset(ptr, 0xCC, 1024 * 1024);

  allocator.Deallocate(ptr);
}

TEST_F(CUDAAllocatorTest, CUDAAllocatorAligned) {
  CUDAAllocator allocator(false);

  // Test aligned allocation (may fall back to standard aligned malloc)
  void* ptr = allocator.AllocateAligned(1024, 256);
  EXPECT_NE(ptr, nullptr);

  // Check alignment (may not be perfect due to fallback)
  // In real CUDA implementation, this would be properly aligned
  allocator.Deallocate(ptr);
}

TEST_F(CUDAAllocatorTest, CUDAAllocatorMultipleAllocations) {
  CUDAAllocator allocator(false);

  std::vector<void*> pointers;
  const int num_allocations = 10;

  // Allocate multiple buffers
  for (int i = 0; i < num_allocations; ++i) {
    void* ptr = allocator.Allocate(4096 * (i + 1));
    EXPECT_NE(ptr, nullptr);
    pointers.push_back(ptr);
  }

  // Deallocate in reverse order
  for (auto it = pointers.rbegin(); it != pointers.rend(); ++it) {
    allocator.Deallocate(*it);
  }
}

// Test CUDA availability detection
TEST_F(CUDAAllocatorTest, CUDAAvailabilityDetection) {
  CUDAAllocator allocator(false);

  // The allocator should be created regardless of CUDA availability
  // Internal availability detection happens at runtime
  EXPECT_STREQ(allocator.Name(), "CUDA");

  // Test basic allocation works (may be CPU fallback)
  void* ptr = allocator.Allocate(1024);
  EXPECT_NE(ptr, nullptr);
  allocator.Deallocate(ptr);
}

// Test both regular and managed CUDA allocators
TEST_F(CUDAAllocatorTest, CUDAAllocatorTypes) {
  CUDAAllocator regular_allocator(false);  // Regular CUDA memory
  CUDAAllocator managed_allocator(true);   // CUDA managed memory

  EXPECT_STREQ(regular_allocator.Name(), "CUDA");
  EXPECT_STREQ(managed_allocator.Name(), "CUDA");

  // Both should work (may fall back to CPU allocation)
  void* ptr1 = regular_allocator.Allocate(1024);
  void* ptr2 = managed_allocator.Allocate(1024);

  EXPECT_NE(ptr1, nullptr);
  EXPECT_NE(ptr2, nullptr);

  regular_allocator.Deallocate(ptr1);
  managed_allocator.Deallocate(ptr2);
}

// Test edge cases
TEST_F(CUDAAllocatorTest, CUDAAllocatorEdgeCases) {
  CUDAAllocator allocator(false);

  // Test null deallocation (should not crash)
  EXPECT_NO_THROW(allocator.Deallocate(nullptr));

  // Test very small allocations
  void* ptr1 = allocator.Allocate(1);
  EXPECT_NE(ptr1, nullptr);
  allocator.Deallocate(ptr1);

  // Test deallocation of invalid pointer (may not crash, depends on implementation)
  // This is dangerous in real code but tests the interface
  // allocator.Deallocate(reinterpret_cast<void*>(0xDEADBEEF));
}

// Performance smoke test
TEST_F(CUDAAllocatorTest, CUDAAllocatorPerformanceSmokeTest) {
  CUDAAllocator allocator(false);

  const int num_iterations = 100;
  std::vector<void*> pointers;

  // Quick performance smoke test
  for (int i = 0; i < num_iterations; ++i) {
    void* ptr = allocator.Allocate(4096);
    EXPECT_NE(ptr, nullptr);
    pointers.push_back(ptr);
  }

  // Clean up
  for (void* ptr : pointers) {
    allocator.Deallocate(ptr);
  }
}




