#include "NovaLLM/data/tensor.h"

#include <gtest/gtest.h>

#if EnableModuleTest

using namespace nova_llm;

class TensorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 测试前的设置
  }

  void TearDown() override {
    // 测试后的清理
  }
};

// 测试默认构造函数
TEST_F(TensorTest, DefaultConstructor) {
  Tensor tensor;
  EXPECT_EQ(tensor.totalElements(), 0);
  EXPECT_TRUE(tensor.dims().empty());
  EXPECT_EQ(tensor.dtype(), DataType::UNKNOWN);
  EXPECT_EQ(tensor.device(), DeviceType::UNKNOWN);
}

// 测试带维度的构造函数
TEST_F(TensorTest, ConstructWithDims) {
  std::vector<uint32_t> dims = {2, 3, 4};
  Tensor tensor(dims, DataType::FLOAT32, DeviceType::CPU);

  EXPECT_EQ(tensor.totalElements(), 24);  // 2*3*4
  EXPECT_EQ(tensor.dims(), dims);
  EXPECT_EQ(tensor.dtype(), DataType::FLOAT32);
  EXPECT_EQ(tensor.device(), DeviceType::CPU);
}

// 测试非法维度
TEST_F(TensorTest, InvalidDimensions) {
  std::vector<uint32_t> empty_dims;
  EXPECT_THROW(Tensor tensor(empty_dims, DataType::FLOAT32, DeviceType::CPU),
               std::invalid_argument);

  std::vector<uint32_t> zero_dims = {2, 0, 4};
  EXPECT_THROW(Tensor tensor(zero_dims, DataType::FLOAT32, DeviceType::CPU), std::invalid_argument);
}

// 测试拷贝构造
TEST_F(TensorTest, CopyConstruction) {
  std::vector<uint32_t> dims = {2, 3};
  Tensor original(dims, DataType::FLOAT32, DeviceType::CPU);
  Tensor copied(original);

  EXPECT_EQ(copied.totalElements(), original.totalElements());
  EXPECT_EQ(copied.dims(), original.dims());
  EXPECT_EQ(copied.dtype(), original.dtype());
  EXPECT_EQ(copied.device(), original.device());
}

// 测试赋值运算符
TEST_F(TensorTest, AssignmentOperator) {
  std::vector<uint32_t> dims = {2, 3};
  Tensor original(dims, DataType::FLOAT32, DeviceType::CPU);
  Tensor assigned;
  assigned = original;

  EXPECT_EQ(assigned.totalElements(), original.totalElements());
  EXPECT_EQ(assigned.dims(), original.dims());
  EXPECT_EQ(assigned.dtype(), original.dtype());
  EXPECT_EQ(assigned.device(), original.device());
}

// 测试内存分配
TEST_F(TensorTest, MemoryAllocation) {
  std::vector<uint32_t> dims = {2, 3};
  Tensor tensor(dims, DataType::FLOAT32, DeviceType::CPU);

  EXPECT_NE(tensor.data(), nullptr);
  EXPECT_EQ(tensor.totalElements(), 6);
}

#endif