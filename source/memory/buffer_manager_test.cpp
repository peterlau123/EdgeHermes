#include "NovaLLM/memory/buffer_manager.h"

#if EnableModuleTest
#include <gtest/gtest.h>

using namespace nova_llm;

class BufferManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BufferManager::Config config;
    // set config
    config.device_flags.set(DeviceType::CPU);
    config.cpu.alloc = std::make_shared<CPUAllocator>();
    config.device_flags.set(DeviceType::NVIDIA_GPU);
    config.gpu.alloc = std::make_shared<GPUAllocator>();

    BufferManager::Builder::build(config);
  }

  void TearDown() override { BufferManager::Builder::getInstance().~BufferManager(); }
};

TEST(BufferManagerTest, Init) {
  auto& buffer_manager = BufferManager::Builder::getInstance();
  EXPECT_TRUE(buffer_manager.isInited());
}

TEST(BufferManagerTest, FetchCpu) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(1024, DeviceType::CPU);

  EXPECT_NE(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 1024);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);

  buffer_manager.put(buffer);
}

TEST(BufferManagerTest, PutCpu) {
  auto& buffer_manager = BufferManager::Builder::getInstance();

  auto buffer = buffer_manager.fetch(1024, DeviceType::CPU);

  buffer_manager.put(buffer);
  EXPECT_EQ(buffer.data, nullptr);
  EXPECT_EQ(buffer.size, 0);
  EXPECT_EQ(buffer.device_type, DeviceType::CPU);
}


#endif