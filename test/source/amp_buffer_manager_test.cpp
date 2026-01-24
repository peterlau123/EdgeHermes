#include "Peregrine/memory/amp_buffer_manager.h"
#include "Peregrine/memory/allocator.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace peregrine;

class AMPBufferManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Note: AMPBufferManager uses singleton pattern, tests should be careful
    // about global state. In a real implementation, we'd want better isolation.
  }

  void TearDown() override {
    // Cleanup is handled by the singleton's lifetime
  }
};

// Test AMPBufferManager construction and initialization
TEST_F(AMPBufferManagerTest, Construction) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);

  // Add CPU allocator
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  EXPECT_NO_THROW({
    AMPBufferManager manager(config);
    EXPECT_TRUE(manager.IsInitialized());
  });
}

// Test Builder::Build method
TEST_F(AMPBufferManagerTest, BuilderBuild) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);
  EXPECT_NE(manager, nullptr);
  EXPECT_TRUE(manager->IsInitialized());
}

// Test basic CPU allocation
TEST_F(AMPBufferManagerTest, FetchCpuSmall) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  Buffer buffer = manager->Fetch(64, DeviceType::CPU);
  EXPECT_NE(buffer.data, nullptr);
  EXPECT_GE(buffer.size, 64);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);

  manager->Put(buffer);
  EXPECT_EQ(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 0);
}

// Test CPU allocation with different sizes
TEST_F(AMPBufferManagerTest, FetchCpuVariousSizes) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  std::vector<size_t> sizes = {1, 64, 512, 4096, 65536};

  for (size_t size : sizes) {
    Buffer buffer = manager->Fetch(size, DeviceType::CPU);
    EXPECT_NE(buffer.data, nullptr);
    EXPECT_GE(buffer.size, size);
    EXPECT_EQ(buffer.device_type, DeviceType::CPU);

    // Verify we can write to the memory
    if (buffer.data) {
      memset(buffer.data, 0xAA, std::min(size, buffer.size));
    }

    manager->Put(buffer);
  }
}

// Test zero size allocation
TEST_F(AMPBufferManagerTest, FetchZeroSize) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  Buffer buffer = manager->Fetch(0, DeviceType::CPU);
  EXPECT_EQ(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 0);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);
}

// Test Put with invalid buffer
TEST_F(AMPBufferManagerTest, PutInvalidBuffer) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  Buffer invalid_buffer{nullptr, 0, DeviceType::CPU};
  EXPECT_NO_THROW(manager->Put(invalid_buffer));
}

// Test multiple allocations and deallocations
TEST_F(AMPBufferManagerTest, MultipleOperations) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  const int num_operations = 100;
  std::vector<Buffer> buffers;

  // Allocate buffers
  for (int i = 0; i < num_operations; ++i) {
    Buffer buffer = manager->Fetch(128, DeviceType::CPU);
    EXPECT_NE(buffer.data, nullptr);
    buffers.push_back(buffer);
  }

  // Deallocate all buffers
  for (auto& buffer : buffers) {
    manager->Put(buffer);
  }

  // Verify all buffers are cleared
  for (const auto& buffer : buffers) {
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_EQ(buffer.size, 0);
  }
}

// Test concurrent access
TEST_F(AMPBufferManagerTest, ConcurrentAccess) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 1024;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  const int num_threads = 4;
  const int operations_per_thread = 50;

  auto thread_func = [&manager]() {
    for (int i = 0; i < operations_per_thread; ++i) {
      Buffer buffer = manager->Fetch(256, DeviceType::CPU);
      EXPECT_NE(buffer.data, nullptr);
      EXPECT_GE(buffer.size, 256);

      // Simulate some work
      std::this_thread::sleep_for(std::chrono::microseconds(10));

      manager->Put(buffer);
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

// Test GetStats functionality
TEST_F(AMPBufferManagerTest, GetStats) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  // Initially should have some stats
  auto initial_stats = manager->GetStats();
  EXPECT_GE(initial_stats.total_allocated, 0);

  // Allocate some memory
  Buffer buffer = manager->Fetch(1024, DeviceType::CPU);
  auto after_alloc_stats = manager->GetStats();
  EXPECT_GE(after_alloc_stats.total_allocated, initial_stats.total_allocated);

  manager->Put(buffer);
}

// Test IsHealthy functionality
TEST_F(AMPBufferManagerTest, IsHealthy) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);
  EXPECT_TRUE(manager->IsHealthy());
}

// Test GetArenaRouter
TEST_F(AMPBufferManagerTest, GetArenaRouter) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);
  EXPECT_NE(manager->GetArenaRouter(), nullptr);
}

// Test different configurations
TEST_F(AMPBufferManagerTest, DifferentConfigurations) {
  std::vector<size_t> cache_sizes = {0, 64, 512, 2048};

  for (size_t cache_size : cache_sizes) {
    AMPBufferManager::Config config;
    config.amp_config.thread_cache_size_kb = cache_size;
    config.device_flags.set(DeviceType::CPU);
    config.allocators[DeviceType::CPU] =
        peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

    auto manager = AMPBufferManager::Builder::Build(config);
    EXPECT_TRUE(manager->IsInitialized());

    // Test basic functionality
    Buffer buffer = manager->Fetch(128, DeviceType::CPU);
    EXPECT_NE(buffer.data, nullptr);
    manager->Put(buffer);
  }
}

// Test edge cases
TEST_F(AMPBufferManagerTest, EdgeCases) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 512;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  // Test very small allocation
  Buffer tiny = manager->Fetch(1, DeviceType::CPU);
  EXPECT_NE(tiny.data, nullptr);
  EXPECT_GE(tiny.size, 1);
  manager->Put(tiny);

  // Test larger allocation
  Buffer large = manager->Fetch(1024 * 1024, DeviceType::CPU);  // 1MB
  if (large.data != nullptr) {
    EXPECT_GE(large.size, 1024 * 1024);
    manager->Put(large);
  }
}

// Test buffer reuse patterns
TEST_F(AMPBufferManagerTest, BufferReuse) {
  AMPBufferManager::Config config;
  config.amp_config.thread_cache_size_kb = 1024;
  config.device_flags.set(DeviceType::CPU);
  config.allocators[DeviceType::CPU] =
      peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

  auto manager = AMPBufferManager::Builder::Build(config);

  // Allocate and deallocate same size multiple times
  for (int i = 0; i < 10; ++i) {
    Buffer buffer = manager->Fetch(256, DeviceType::CPU);
    EXPECT_NE(buffer.data, nullptr);

    // Fill with pattern
    memset(buffer.data, static_cast<uint8_t>(i), 256);

    manager->Put(buffer);
  }
}

// Test destructor cleanup
TEST_F(AMPBufferManagerTest, DestructorCleanup) {
  // Create manager in scope
  {
    AMPBufferManager::Config config;
    config.amp_config.thread_cache_size_kb = 512;
    config.device_flags.set(DeviceType::CPU);
    config.allocators[DeviceType::CPU] =
        peregrine::amp::AllocatorFactory::Create(peregrine::amp::AllocatorType::STANDARD);

    auto manager = AMPBufferManager::Builder::Build(config);

    // Allocate some buffers
    std::vector<Buffer> buffers;
    for (int i = 0; i < 5; ++i) {
      buffers.push_back(manager->Fetch(128, DeviceType::CPU));
    }

    // Don't explicitly deallocate - destructor should handle cleanup
  }
  // Should not crash on destruction
  SUCCEED();
}




