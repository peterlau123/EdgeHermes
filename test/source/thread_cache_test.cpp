#include "NovaLLM/memory/thread_cache.h"
#include "memory/thread_cache_storage.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace nova_llm::amp;

class ThreadCacheTest : public ::testing::Test {
 protected:
  void SetUp() override {
    size_class_system_ = &GetSizeClassSystem();
    config_.thread_cache_size_kb = 512;
  }

  void TearDown() override {
    // Cleanup after each test
  }

  const SizeClassSystem* size_class_system_;
  AMPConfig config_;
};

// Test ThreadCache construction and destruction
TEST_F(ThreadCacheTest, ConstructionDestruction) {
  EXPECT_NO_THROW({
    ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);
  });
}

// Test basic allocation with empty batch allocation (current implementation)
TEST_F(ThreadCacheTest, AllocateWithEmptyBatch) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Since BatchAllocate returns empty, Allocate should return nullptr
  void* ptr = cache.Allocate(0);  // Small size class
  EXPECT_EQ(ptr, nullptr);
}

// Test deallocate with nullptr
TEST_F(ThreadCacheTest, DeallocateNullptr) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Deallocate nullptr should return false
  bool result = cache.Deallocate(nullptr, 0);
  EXPECT_FALSE(result);
}

// Test deallocate with invalid size class
TEST_F(ThreadCacheTest, DeallocateInvalidSizeClass) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  char dummy[64];
  bool result = cache.Deallocate(&dummy, ThreadCache::MAX_SIZE_CLASSES);
  EXPECT_FALSE(result);
}

// Test cache statistics
TEST_F(ThreadCacheTest, InitialStats) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  auto stats = cache.GetStats();
  EXPECT_EQ(stats.total_objects, 0);
  EXPECT_EQ(stats.total_bytes, 0);
  EXPECT_EQ(stats.hits, 0);
  EXPECT_EQ(stats.misses, 0);
}

// Test IsFull method
TEST_F(ThreadCacheTest, IsFullCheck) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Initially not full
  EXPECT_FALSE(cache.IsFull(0));

  // Invalid size class should be considered full
  EXPECT_TRUE(cache.IsFull(ThreadCache::MAX_SIZE_CLASSES));
}

// Test Flush operation
TEST_F(ThreadCacheTest, Flush) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Flush should not crash
  EXPECT_NO_THROW(cache.Flush());
}

// Test ThreadCacheStorage initialization
TEST_F(ThreadCacheTest, ThreadCacheStorageInitialize) {
  EXPECT_NO_THROW({
    ThreadCacheStorage::Initialize(*size_class_system_, config_);
  });
}

// Test ThreadCacheStorage Get without initialization (should throw)
TEST_F(ThreadCacheTest, ThreadCacheStorageGetUninitialized) {
  // Cleanup first
  ThreadCacheStorage::Cleanup();

  EXPECT_THROW({
    ThreadCacheStorage::Get();
  }, std::runtime_error);
}

// Test ThreadCacheStorage Get after initialization
TEST_F(ThreadCacheTest, ThreadCacheStorageGetInitialized) {
  ThreadCacheStorage::Initialize(*size_class_system_, config_);

  EXPECT_NO_THROW({
    ThreadCache& cache = ThreadCacheStorage::Get();
    // Verify we get a valid cache
    EXPECT_NE(&cache, nullptr);
  });

  ThreadCacheStorage::Cleanup();
}

// Test ThreadCacheStorage Cleanup
TEST_F(ThreadCacheTest, ThreadCacheStorageCleanup) {
  ThreadCacheStorage::Initialize(*size_class_system_, config_);
  ThreadCacheStorage::Get();  // Create cache instance

  EXPECT_NO_THROW({
    ThreadCacheStorage::Cleanup();
  });

  // After cleanup, Get should throw again
  EXPECT_THROW({
    ThreadCacheStorage::Get();
  }, std::runtime_error);
}

// Test thread-local behavior (basic check)
TEST_F(ThreadCacheTest, ThreadLocalBehavior) {
  ThreadCacheStorage::Initialize(*size_class_system_, config_);

  ThreadCache& cache1 = ThreadCacheStorage::Get();
  ThreadCache& cache2 = ThreadCacheStorage::Get();

  // Should be the same instance within the same thread
  EXPECT_EQ(&cache1, &cache2);

  ThreadCacheStorage::Cleanup();
}

// Test statistics tracking with mock allocations/deallocations
TEST_F(ThreadCacheTest, StatisticsTracking) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Initially zero
  auto initial_stats = cache.GetStats();
  EXPECT_EQ(initial_stats.hits, 0);
  EXPECT_EQ(initial_stats.misses, 0);

  // Allocate (will miss since BatchAllocate returns empty)
  cache.Allocate(0);
  auto after_miss_stats = cache.GetStats();
  EXPECT_EQ(after_miss_stats.hits, 0);
  EXPECT_EQ(after_miss_stats.misses, 1);

  // Try to deallocate something (will fail since cache is empty)
  char dummy[64];
  cache.Deallocate(&dummy, 0);
  // Stats should remain the same since deallocate failed
  auto final_stats = cache.GetStats();
  EXPECT_EQ(final_stats.hits, 0);
  EXPECT_EQ(final_stats.misses, 1);
}

// Test edge cases for size classes
TEST_F(ThreadCacheTest, SizeClassBounds) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Valid size classes
  for (size_t i = 0; i < ThreadCache::MAX_SIZE_CLASSES; ++i) {
    EXPECT_NO_THROW(cache.Allocate(i));
    EXPECT_FALSE(cache.IsFull(i));
  }

  // Invalid size class
  EXPECT_EQ(cache.Allocate(ThreadCache::MAX_SIZE_CLASSES), nullptr);
  EXPECT_TRUE(cache.IsFull(ThreadCache::MAX_SIZE_CLASSES));
}

// Test multiple allocations and deallocations
TEST_F(ThreadCacheTest, MultipleOperations) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Perform multiple operations
  for (int i = 0; i < 10; ++i) {
    cache.Allocate(i % ThreadCache::MAX_SIZE_CLASSES);
  }

  auto stats = cache.GetStats();
  EXPECT_EQ(stats.misses, 10);
  EXPECT_EQ(stats.hits, 0);
}

// Test cache capacity limits (though hard to test fully with placeholder implementation)
TEST_F(ThreadCacheTest, CacheLimits) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Test with zero cache size
  ThreadCache zero_cache(*size_class_system_, 0);
  EXPECT_NO_THROW(zero_cache.Allocate(0));
}

// Test destructor cleanup
TEST_F(ThreadCacheTest, DestructorCleanup) {
  // Create cache in scope and let it go out of scope
  {
    ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);
    cache.Allocate(0);  // Add some operations
  }
  // Should not crash on destruction
  SUCCEED();
}

// Test concurrent access patterns (basic)
TEST_F(ThreadCacheTest, ConcurrentInitialization) {
  // Test that multiple threads can initialize safely
  std::atomic<bool> initialized{false};
  std::atomic<int> ready_count{0};

  auto thread_func = [&]() {
    ready_count++;
    while (ready_count.load() < 2) {
      std::this_thread::yield();
    }

    if (!initialized.exchange(true)) {
      ThreadCacheStorage::Initialize(*size_class_system_, config_);
    }

    ThreadCache& cache = ThreadCacheStorage::Get();
    EXPECT_NE(&cache, nullptr);
  };

  std::thread t1(thread_func);
  std::thread t2(thread_func);

  t1.join();
  t2.join();

  ThreadCacheStorage::Cleanup();
}

// Test configuration variations
TEST_F(ThreadCacheTest, DifferentConfigurations) {
  std::vector<size_t> cache_sizes = {0, 1, 64, 512, 1024, 4096};

  for (size_t cache_size : cache_sizes) {
    ThreadCache cache(*size_class_system_, cache_size);
    EXPECT_NO_THROW(cache.Allocate(0));
    auto stats = cache.GetStats();
    EXPECT_EQ(stats.hits, 0);  // Will always miss with current implementation
  }
}

// Test that cache handles different size classes independently
TEST_F(ThreadCacheTest, SizeClassIsolation) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Allocate from different size classes
  for (size_t class_id = 0; class_id < std::min(size_t(5), ThreadCache::MAX_SIZE_CLASSES); ++class_id) {
    cache.Allocate(class_id);
  }

  auto stats = cache.GetStats();
  EXPECT_EQ(stats.misses, 5);
  EXPECT_EQ(stats.hits, 0);
}

// Test boundary conditions
TEST_F(ThreadCacheTest, BoundaryConditions) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Test with first and last valid size classes
  EXPECT_EQ(cache.Allocate(0), nullptr);
  if (ThreadCache::MAX_SIZE_CLASSES > 0) {
    EXPECT_EQ(cache.Allocate(ThreadCache::MAX_SIZE_CLASSES - 1), nullptr);
  }

  // Test deallocate bounds
  char dummy[64];
  EXPECT_FALSE(cache.Deallocate(&dummy, 0));
  EXPECT_FALSE(cache.Deallocate(&dummy, ThreadCache::MAX_SIZE_CLASSES - 1));
}

// Test that operations are idempotent where expected
TEST_F(ThreadCacheTest, IdempotentOperations) {
  ThreadCache cache(*size_class_system_, config_.thread_cache_size_kb);

  // Multiple flushes should be safe
  cache.Flush();
  cache.Flush();
  cache.Flush();

  // Multiple stats queries should be safe
  auto stats1 = cache.GetStats();
  auto stats2 = cache.GetStats();
  EXPECT_EQ(stats1.hits, stats2.hits);
  EXPECT_EQ(stats1.misses, stats2.misses);
}

// Test ThreadCacheStorage re-initialization
TEST_F(ThreadCacheTest, ThreadCacheStorageReinitialize) {
  ThreadCacheStorage::Initialize(*size_class_system_, config_);
  ThreadCache& cache1 = ThreadCacheStorage::Get();

  ThreadCacheStorage::Cleanup();

  // Re-initialize with different config
  AMPConfig new_config = config_;
  new_config.thread_cache_size_kb = 1024;
  ThreadCacheStorage::Initialize(*size_class_system_, new_config);
  ThreadCache& cache2 = ThreadCacheStorage::Get();

  // Should be different instances
  EXPECT_NE(&cache1, &cache2);

  ThreadCacheStorage::Cleanup();
}

// Test error handling in ThreadCacheStorage
TEST_F(ThreadCacheTest, ThreadCacheStorageErrorHandling) {
  // Test cleanup without initialization
  EXPECT_NO_THROW(ThreadCacheStorage::Cleanup());

  // Test multiple cleanups
  ThreadCacheStorage::Initialize(*size_class_system_, config_);
  ThreadCacheStorage::Cleanup();
  EXPECT_NO_THROW(ThreadCacheStorage::Cleanup());
}
