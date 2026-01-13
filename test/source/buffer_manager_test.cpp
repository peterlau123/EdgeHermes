#include "EdgeHermes/memory/buffer_manager.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace edgehermes;

class BufferManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clean up any existing instance
    BufferManager::Builder::getInstance().destroy();

    BufferManager::Config config;
    config.device_flags.set(DeviceType::CPU);
    // Note: AMP system uses internal allocators, legacy config.cpu/gpu.alloc is ignored

    BufferManager::Builder::build(config);
  }

  void TearDown() override {
    BufferManager::Builder::getInstance().destroy();
  }
};

// Basic initialization tests
TEST_F(BufferManagerTest, Init) {
  auto& buffer_manager = BufferManager::Builder::getInstance();
  EXPECT_TRUE(buffer_manager.isInited());
}

TEST_F(BufferManagerTest, DoubleInit) {
  auto& buffer_manager1 = BufferManager::Builder::getInstance();
  auto& buffer_manager2 = BufferManager::Builder::getInstance();

  // Should return the same instance
  EXPECT_EQ(&buffer_manager1, &buffer_manager2);
  EXPECT_TRUE(buffer_manager1.isInited());
}

// CPU memory allocation tests
TEST_F(BufferManagerTest, FetchCpuSmall) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(64, DeviceType::CPU);

  EXPECT_NE(buffer.data, nullptr);
  EXPECT_GE(buffer.size, 64);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);

  buffer_manager.put(buffer);
}

TEST_F(BufferManagerTest, FetchCpuMedium) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(4096, DeviceType::CPU);

  EXPECT_NE(buffer.data, nullptr);
  EXPECT_GE(buffer.size, 4096);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);

  buffer_manager.put(buffer);
}

TEST_F(BufferManagerTest, FetchCpuLarge) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(1024 * 1024, DeviceType::CPU);  // 1MB

  EXPECT_NE(buffer.data, nullptr);
  EXPECT_GE(buffer.size, 1024 * 1024);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);

  buffer_manager.put(buffer);
}

TEST_F(BufferManagerTest, PutCpu) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(1024, DeviceType::CPU);
  ASSERT_NE(buffer.data, nullptr);

  buffer_manager.put(buffer);

  // Buffer should be cleared after put
  EXPECT_EQ(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 0);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);
}

TEST_F(BufferManagerTest, PutInvalidBuffer) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  Buffer invalid_buffer{nullptr, 0, DeviceType::CPU};
  // Should not crash
  EXPECT_NO_THROW(buffer_manager.put(invalid_buffer));
}

TEST_F(BufferManagerTest, FetchZeroSize) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(0, DeviceType::CPU);

  // Should return empty buffer for zero size
  EXPECT_EQ(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 0);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);
}

// Multiple allocation tests
TEST_F(BufferManagerTest, MultipleAllocations) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  const int num_allocations = 100;
  std::vector<Buffer> buffers;

  // Allocate multiple buffers
  for (int i = 0; i < num_allocations; ++i) {
    auto buffer = buffer_manager.fetch(128 * (i + 1), DeviceType::CPU);
    EXPECT_NE(buffer.data, nullptr);
    buffers.push_back(buffer);
  }

  // Deallocate in reverse order
  for (auto it = buffers.rbegin(); it != buffers.rend(); ++it) {
    buffer_manager.put(*it);
  }
}

// Concurrent access tests
TEST_F(BufferManagerTest, ConcurrentAccess) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  const int num_threads = 4;
  const int allocations_per_thread = 50;

  auto thread_func = [&buffer_manager]() {
    for (int i = 0; i < allocations_per_thread; ++i) {
      auto buffer = buffer_manager.fetch(256, DeviceType::CPU);
      EXPECT_NE(buffer.data, nullptr);
      EXPECT_GE(buffer.size, 256);
      EXPECT_EQ(buffer.device_type, DeviceType::CPU);

      // Simulate some work
      std::this_thread::sleep_for(std::chrono::microseconds(1));

      buffer_manager.put(buffer);
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

// Memory leak detection test
TEST_F(BufferManagerTest, MemoryAccounting) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  // This is a basic smoke test - comprehensive leak detection
  // would require integration with memory profiling tools

  const int num_allocations = 1000;
  std::vector<Buffer> active_buffers;

  // Allocate buffers
  for (int i = 0; i < num_allocations; ++i) {
    auto buffer = buffer_manager.fetch(64, DeviceType::CPU);
    active_buffers.push_back(buffer);
  }

  // Deallocate all buffers
  for (auto& buffer : active_buffers) {
    buffer_manager.put(buffer);
  }

  active_buffers.clear();

  // System should still be functional
  auto test_buffer = buffer_manager.fetch(1024, DeviceType::CPU);
  EXPECT_NE(test_buffer.data, nullptr);
  buffer_manager.put(test_buffer);
}

// Edge case tests
TEST_F(BufferManagerTest, VeryLargeAllocation) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  // Try allocating a very large buffer (may fail, but shouldn't crash)
  auto buffer = buffer_manager.fetch(100 * 1024 * 1024, DeviceType::CPU);  // 100MB

  // If allocation succeeds, clean it up
  if (buffer.data != nullptr) {
    buffer_manager.put(buffer);
  }
  // If it fails, that's also acceptable for this test
}

TEST_F(BufferManagerTest, RapidAllocDealloc) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  // Rapid alloc/dealloc cycle to stress test the system
  for (int cycle = 0; cycle < 10; ++cycle) {
    std::vector<Buffer> buffers;
    for (int i = 0; i < 20; ++i) {
      auto buffer = buffer_manager.fetch(128, DeviceType::CPU);
      EXPECT_NE(buffer.data, nullptr);
      buffers.push_back(buffer);
    }

    for (auto& buffer : buffers) {
      buffer_manager.put(buffer);
    }
  }
}



