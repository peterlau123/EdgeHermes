#include "NovaLLM/memory/buffer_hub.h"

#include <gtest/gtest.h>

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
