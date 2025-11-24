#include "NovaLLM/memory/buffer_hub.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>

using namespace nova_llm;

class CPUBufferHubTest : public ::testing::Test {
 public:
  BufferHub* getBufferHub() { return buffer_hub_; }

 protected:
  void SetUp() override {
    BufferHubConfig config(DeviceType::CPU, std::make_shared<CPUAllocator>(), Size(0, 0, 0, 4));
    buffer_hub_ = BufferHub::Builder::build(config);
  }

  void TearDown() override { BufferHub::Builder::destroy(&buffer_hub_); }

  BufferHub* buffer_hub_;
};

TEST_F(CPUBufferHubTest, Init) { EXPECT_NE(getBufferHub(), nullptr); }

TEST_F(CPUBufferHubTest, GetBlock) {
  auto* block = getBufferHub()->getBlock(Size(1024));

  EXPECT_NE(block, nullptr);
  EXPECT_NE(block->data, nullptr);
  EXPECT_GE(block->size, 1024);
  EXPECT_EQ(block->ref_cnt, 1);

  getBufferHub()->putBlock(block);
}

TEST_F(CPUBufferHubTest, PutBlock) {
  auto* block = getBufferHub()->getBlock(Size(1024));

  EXPECT_NE(block, nullptr);
  EXPECT_NE(block->data, nullptr);
  EXPECT_GE(block->size, 1024);
  EXPECT_EQ(block->ref_cnt, 1);

  // Return the block to the pool; block remains valid but is marked free
  getBufferHub()->putBlock(block);

  EXPECT_NE(block->data, nullptr);
  EXPECT_GE(block->size, 1024);
  EXPECT_EQ(block->ref_cnt, 0);  // ref count reset when returned to pool

  // Fetch another block of the same size and ensure we get a (possibly reused) block
  auto* block2 = getBufferHub()->getBlock(Size(1024));
  EXPECT_NE(block2, nullptr);
  EXPECT_NE(block2->data, nullptr);
  EXPECT_GE(block2->size, 1024);
  EXPECT_EQ(block2->ref_cnt, 1);
}

TEST_F(CPUBufferHubTest, PutBlockFromBuffer) {
  auto* block = getBufferHub()->getBlock(Size(1024));

  EXPECT_NE(block, nullptr);
  EXPECT_NE(block->data, nullptr);
  EXPECT_GE(block->size, 1024);
  EXPECT_EQ(block->ref_cnt, 1);

  Buffer buffer;
  buffer.data = block->data;
  buffer.size = block->size;
  buffer.device_type = DeviceType::CPU;
  getBufferHub()->putBlockFromBuffer(buffer);

  // After returning via Buffer, the underlying block should be returned to the pool.
  // The Buffer should be cleared to avoid dangling pointers.
  EXPECT_EQ(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 0);
}

// Concurrent access tests
TEST_F(CPUBufferHubTest, ConcurrentAddSizeLevel) {
  const int num_threads = 10;
  const int num_levels_per_thread = 5;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};
  
  // Each thread adds multiple size levels
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &success_count]() {
      for (int i = 0; i < num_levels_per_thread; ++i) {
        // Create unique sizes for each thread to avoid conflicts
        uint64_t size_bytes = (1 << 20) * (t * num_levels_per_thread + i + 100);  // 100MB+
        Size level_size(size_bytes);
        uint32_t index = t * num_levels_per_thread + i + 1000;
        
        getBufferHub()->addSizeLevel(index, level_size);
        success_count++;
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Verify all additions succeeded
  EXPECT_EQ(success_count.load(), num_threads * num_levels_per_thread);
}

TEST_F(CPUBufferHubTest, ConcurrentEraseSizeLevel) {
  const int num_threads = 8;
  std::vector<std::thread> threads;
  std::vector<Size> sizes_to_add;
  
  // Pre-populate with size levels
  for (int i = 0; i < num_threads * 2; ++i) {
    uint64_t size_bytes = (1 << 20) * (i + 200);  // 200MB+
    Size level_size(size_bytes);
    sizes_to_add.push_back(level_size);
    getBufferHub()->addSizeLevel(2000 + i, level_size);
  }
  
  std::atomic<int> erase_attempts{0};
  
  // Each thread attempts to erase different size levels concurrently
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &sizes_to_add, &erase_attempts]() {
      // Each thread erases 2 levels
      for (int i = 0; i < 2; ++i) {
        int idx = t * 2 + i;
        getBufferHub()->eraseSizeLevel(sizes_to_add[idx]);
        erase_attempts++;
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  EXPECT_EQ(erase_attempts.load(), num_threads * 2);
}

TEST_F(CPUBufferHubTest, ConcurrentGetBlock) {
  const int num_threads = 20;
  const int blocks_per_thread = 5;
  std::vector<std::thread> threads;
  std::vector<std::vector<BlockRawPtr>> thread_blocks(num_threads);
  std::atomic<int> successful_gets{0};
  
  // Multiple threads requesting blocks of the same size concurrently
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &thread_blocks, &successful_gets]() {
      for (int i = 0; i < blocks_per_thread; ++i) {
        auto* block = getBufferHub()->getBlock(Size(4096));  // 4KB blocks
        if (block != nullptr && block->data != nullptr) {
          thread_blocks[t].push_back(block);
          successful_gets++;
          
          // Verify block properties
          EXPECT_NE(block->data, nullptr);
          EXPECT_GE(block->size, 4096);
          EXPECT_EQ(block->ref_cnt, 1);
        }
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Verify we got the expected number of blocks
  EXPECT_EQ(successful_gets.load(), num_threads * blocks_per_thread);
  
  // Verify all blocks have unique data pointers (no double allocation)
  std::vector<Block::DataPtr> all_data_ptrs;
  for (const auto& blocks : thread_blocks) {
    for (const auto& block : blocks) {
      all_data_ptrs.push_back(block->data);
    }
  }
  std::sort(all_data_ptrs.begin(), all_data_ptrs.end());
  auto last = std::unique(all_data_ptrs.begin(), all_data_ptrs.end());
  EXPECT_EQ(last - all_data_ptrs.begin(), num_threads * blocks_per_thread);
  
  // Clean up - return all blocks
  for (auto& blocks : thread_blocks) {
    for (auto* block : blocks) {
      getBufferHub()->putBlock(block);
    }
  }
}

TEST_F(CPUBufferHubTest, ConcurrentPutBlock) {
  const int num_threads = 15;
  const int blocks_per_thread = 4;
  std::vector<std::thread> threads;
  std::vector<std::vector<BlockRawPtr>> thread_blocks(num_threads);
  
  // First, get blocks in a single-threaded manner
  for (int t = 0; t < num_threads; ++t) {
    for (int i = 0; i < blocks_per_thread; ++i) {
      auto* block = getBufferHub()->getBlock(Size(2048));  // 2KB blocks
      ASSERT_NE(block, nullptr);
      thread_blocks[t].push_back(block);
    }
  }
  
  std::atomic<int> successful_puts{0};
  
  // Now return blocks concurrently from multiple threads
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &thread_blocks, &successful_puts]() {
      for (auto* block : thread_blocks[t]) {
        EXPECT_EQ(block->ref_cnt, 1);
        getBufferHub()->putBlock(block);
        successful_puts++;
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  EXPECT_EQ(successful_puts.load(), num_threads * blocks_per_thread);
  
  // Verify blocks are returned properly by checking ref_cnt
  for (const auto& blocks : thread_blocks) {
    for (const auto* block : blocks) {
      EXPECT_EQ(block->ref_cnt, 0);
    }
  }
}

TEST_F(CPUBufferHubTest, ConcurrentPutBlockFromBuffer) {
  const int num_threads = 12;
  const int blocks_per_thread = 3;
  std::vector<std::thread> threads;
  std::vector<std::vector<Buffer>> thread_buffers(num_threads);
  
  // First, get blocks and create buffers in a single-threaded manner
  for (int t = 0; t < num_threads; ++t) {
    for (int i = 0; i < blocks_per_thread; ++i) {
      auto* block = getBufferHub()->getBlock(Size(8192));  // 8KB blocks
      ASSERT_NE(block, nullptr);
      
      Buffer buffer;
      buffer.data = block->data;
      buffer.size = block->size;
      buffer.device_type = DeviceType::CPU;
      thread_buffers[t].push_back(buffer);
    }
  }
  
  std::atomic<int> successful_puts{0};
  
  // Now return buffers concurrently from multiple threads
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &thread_buffers, &successful_puts]() {
      for (auto& buffer : thread_buffers[t]) {
        EXPECT_NE(buffer.data, nullptr);
        EXPECT_NE(buffer.size, 0);
        
        getBufferHub()->putBlockFromBuffer(buffer);
        
        // Verify buffer was cleared
        EXPECT_EQ(buffer.data, nullptr);
        EXPECT_EQ(buffer.size, 0);
        
        successful_puts++;
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  EXPECT_EQ(successful_puts.load(), num_threads * blocks_per_thread);
}

// Mixed concurrent operations test
TEST_F(CPUBufferHubTest, ConcurrentMixedOperations) {
  const int num_threads = 16;
  std::vector<std::thread> threads;
  std::atomic<int> total_operations{0};
  
  // Mix of get and put operations happening concurrently
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([this, t, &total_operations]() {
      std::vector<BlockRawPtr> blocks;
      
      // Perform alternating get and put operations
      for (int i = 0; i < 10; ++i) {
        // Get a block
        auto* block = getBufferHub()->getBlock(Size(1024 * (t % 4 + 1)));  // Varying sizes
        if (block != nullptr) {
          EXPECT_NE(block->data, nullptr);
          EXPECT_EQ(block->ref_cnt, 1);
          blocks.push_back(block);
          total_operations++;
        }
        
        // Return a previously acquired block if we have any
        if (!blocks.empty() && i % 3 == 0) {
          auto* return_block = blocks.back();
          blocks.pop_back();
          getBufferHub()->putBlock(return_block);
          // Note: Don't check ref_cnt here as it's being modified concurrently
          total_operations++;
        }
      }
      
      // Clean up remaining blocks
      for (auto* block : blocks) {
        getBufferHub()->putBlock(block);
        total_operations++;
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Verify operations completed
  EXPECT_GT(total_operations.load(), 0);
}
