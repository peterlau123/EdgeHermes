#include "NovaLLM/memory/buffer_hub.h"

#include <gtest/gtest.h>

using namespace nova_llm;

class CPUBufferHubTest : public ::testing::Test {
public:
    BufferHub* getBufferHub(){
        return buffer_hub_;
    }

protected:
    void SetUp() override {
        BufferHub::Config config;
        //set config
        config.device_type = DeviceType::CPU;
        config.size_levels=std::vector<Size>{Size(0, 0, 0, 1)};
        config.allocator = std::make_shared<CPUAllocator>();

        buffer_hub_ = BufferHub::Builder::build(config);
    }

    void TearDown() override {
        BufferHub::Builder::destroy(&buffer_hub_);
    }
    BufferHub* buffer_hub_;
};

TEST_F(CPUBufferHubTest, Init) {
    EXPECT_NE(getBufferHub(), nullptr);
}

TEST_F(CPUBufferHubTest, GetBlock) {
    auto *block = getBufferHub()->getBlock(Size(1024));

    EXPECT_NE(block, nullptr);
    EXPECT_EQ(block->data, nullptr);
    EXPECT_EQ(block->size, 1024);
    EXPECT_EQ(block->ref_cnt, 1);

    getBufferHub()->putBlock(block);
}

TEST_F(CPUBufferHubTest, PutBlock) {
    auto* block = getBufferHub()->getBlock(Size(1024));

    EXPECT_NE(block, nullptr);
    EXPECT_NE(block->data, nullptr);
    EXPECT_EQ(block->size, 1024);
    EXPECT_EQ(block->ref_cnt, 1);

    getBufferHub()->putBlock(block);

    EXPECT_EQ(block->data, nullptr);
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->ref_cnt, 0);
}


TEST_F(CPUBufferHubTest, PutBlockFromBuffer) {
    auto* block = getBufferHub()->getBlock(Size(1024));

    EXPECT_NE(block, nullptr);
    EXPECT_NE(block->data, nullptr);
    EXPECT_EQ(block->size, 1024);
    EXPECT_EQ(block->ref_cnt, 1);

    Buffer buffer;
    buffer.data = block->data;
    buffer.size = block->size;
    buffer.device_type = DeviceType::CPU;
    getBufferHub()->putBlockFromBuffer(buffer);

    EXPECT_EQ(block->data, nullptr);
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->ref_cnt, 0);

    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_EQ(buffer.size, 0);
}