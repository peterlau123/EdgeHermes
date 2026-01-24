#include "Peregrine/memory/allocator.h"

#include <gtest/gtest.h>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace peregrine::amp;

class AllocatorWrapperTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test StandardAllocator basic functionality
TEST_F(AllocatorWrapperTest, StandardAllocatorBasic) {
  StandardAllocator allocator;

  EXPECT_STREQ(allocator.Name(), "Standard");

  // Test allocation and deallocation
  void* ptr = allocator.Allocate(1024);
  EXPECT_NE(ptr, nullptr);

  // Should be able to write to the memory
  memset(ptr, 0xAA, 1024);

  allocator.Deallocate(ptr);
}

TEST_F(AllocatorWrapperTest, StandardAllocatorZeroSize) {
  StandardAllocator allocator;

  void* ptr = allocator.Allocate(0);
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(AllocatorWrapperTest, StandardAllocatorAligned) {
  StandardAllocator allocator;

  // Test aligned allocation
  void* ptr = allocator.AllocateAligned(1024, 64);
  EXPECT_NE(ptr, nullptr);

  // Check alignment
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0);

  allocator.Deallocate(ptr);
}

// Test AllocatorFactory
TEST_F(AllocatorWrapperTest, FactoryCreateStandard) {
  auto allocator = AllocatorFactory::Create(AllocatorType::STANDARD);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "Standard");
}

TEST_F(AllocatorWrapperTest, FactoryCreateTCMalloc) {
  auto allocator = AllocatorFactory::Create(AllocatorType::TCMALLOC);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "TCMalloc");
}

TEST_F(AllocatorWrapperTest, FactoryCreateJemalloc) {
  auto allocator = AllocatorFactory::Create(AllocatorType::JEMALLOC);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "Jemalloc");
}

TEST_F(AllocatorWrapperTest, FactoryCreateMimalloc) {
  auto allocator = AllocatorFactory::Create(AllocatorType::MIMALLOC);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "Mimalloc");
}

// CUDA allocator tests have been moved to cuda_allocator_test.cpp

TEST_F(AllocatorWrapperTest, FactoryGetAllocatorName) {
  EXPECT_STREQ(AllocatorFactory::GetAllocatorName(AllocatorType::STANDARD), "Standard");
  EXPECT_STREQ(AllocatorFactory::GetAllocatorName(AllocatorType::TCMALLOC), "TCMalloc");
  EXPECT_STREQ(AllocatorFactory::GetAllocatorName(AllocatorType::JEMALLOC), "Jemalloc");
  EXPECT_STREQ(AllocatorFactory::GetAllocatorName(AllocatorType::MIMALLOC), "Mimalloc");
}

TEST_F(AllocatorWrapperTest, FactoryIsAvailable) {
  // Standard allocator is always available
  EXPECT_TRUE(AllocatorFactory::IsAvailable(AllocatorType::STANDARD));

  // Third-party allocators may not be available (depending on build)
  // We don't test these as they depend on external libraries
}

TEST_F(AllocatorWrapperTest, FactoryGetAvailableAllocators) {
  auto available = AllocatorFactory::GetAvailableAllocators();
  EXPECT_FALSE(available.empty());
  EXPECT_EQ(available[0], AllocatorType::STANDARD);
}

// Test TCMallocAllocator with options
TEST_F(AllocatorWrapperTest, TCMallocWithOptions) {
  std::unordered_map<std::string, std::string> options = {
    {"max_cache_size", "67108864"},  // 64MB
    {"background_threads", "4"}
  };

  auto allocator = AllocatorFactory::Create(AllocatorType::TCMALLOC, options);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "TCMalloc");

  // Test basic functionality (may fall back to standard malloc)
  void* ptr = allocator->Allocate(1024);
  EXPECT_NE(ptr, nullptr);
  allocator->Deallocate(ptr);
}

// Test JemallocAllocator with options
TEST_F(AllocatorWrapperTest, JemallocWithOptions) {
  std::unordered_map<std::string, std::string> options = {
    {"narenas", "4"},
    {"dirty_decay_ms", "10000"}
  };

  auto allocator = AllocatorFactory::Create(AllocatorType::JEMALLOC, options);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "Jemalloc");

  // Test basic functionality (may fall back to standard malloc)
  void* ptr = allocator->Allocate(1024);
  EXPECT_NE(ptr, nullptr);
  allocator->Deallocate(ptr);
}

// Test MimallocAllocator with options
TEST_F(AllocatorWrapperTest, MimallocWithOptions) {
  std::unordered_map<std::string, std::string> options = {
    {"heap_grow_factor", "2.0"},
    {"heap_max_size", "1073741824"}  // 1GB
  };

  auto allocator = AllocatorFactory::Create(AllocatorType::MIMALLOC, options);
  EXPECT_NE(allocator, nullptr);
  EXPECT_STREQ(allocator->Name(), "Mimalloc");

  // Test basic functionality (may fall back to standard malloc)
  void* ptr = allocator->Allocate(1024);
  EXPECT_NE(ptr, nullptr);
  allocator->Deallocate(ptr);
}



// Test memory allocation patterns
TEST_F(AllocatorWrapperTest, AllocationPatterns) {
  auto allocator = AllocatorFactory::Create(AllocatorType::STANDARD);

  // Test various allocation sizes
  std::vector<size_t> sizes = {1, 8, 64, 512, 4096, 32768, 262144};

  for (size_t size : sizes) {
    void* ptr = allocator->Allocate(size);
    EXPECT_NE(ptr, nullptr);

    // Fill with pattern
    memset(ptr, 0xBB, size);

    allocator->Deallocate(ptr);
  }
}

TEST_F(AllocatorWrapperTest, AlignedAllocation) {
  auto allocator = AllocatorFactory::Create(AllocatorType::STANDARD);

  std::vector<size_t> alignments = {1, 2, 4, 8, 16, 32, 64, 128};

  for (size_t alignment : alignments) {
    void* ptr = allocator->AllocateAligned(1024, alignment);
    if (ptr != nullptr) {
      // Check alignment
      EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignment, 0);
      allocator->Deallocate(ptr);
    }
  }
}

// Test concurrent allocations (basic smoke test)
TEST_F(AllocatorWrapperTest, ConcurrentAllocations) {
  auto allocator = AllocatorFactory::Create(AllocatorType::STANDARD);

  const int num_threads = 4;
  const int allocations_per_thread = 100;

  auto thread_func = [&allocator]() {
    for (int i = 0; i < allocations_per_thread; ++i) {
      void* ptr = allocator->Allocate(128);
      EXPECT_NE(ptr, nullptr);

      // Quick memset to ensure memory is writable
      memset(ptr, 0xCC, 128);

      allocator->Deallocate(ptr);
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(thread_func);
  }

  for (auto& thread : threads) {
    thread.join();
  }
}




