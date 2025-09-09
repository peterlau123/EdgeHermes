#include "NovaLLM/data/tensor.h"

#include <gtest/gtest.h>

namespace nova_llm {
namespace test {

class TensorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code that will be called before each test
  }

  void TearDown() override {
    // Cleanup code that will be called after each test
  }
};

TEST_F(TensorTest, CreateTensorTest) {
  /*Tensor tensor({2, 3});
  EXPECT_EQ(tensor.dimensions().size(), 2);
  EXPECT_EQ(tensor.dimensions()[0], 2);
  EXPECT_EQ(tensor.dimensions()[1], 3);
  */
}

TEST_F(TensorTest, SetAndGetValueTest) {
  /*Tensor tensor({2, 2});
  tensor.setValue({0, 0}, 1.0f);
  tensor.setValue({0, 1}, 2.0f);
  tensor.setValue({1, 0}, 3.0f);
  tensor.setValue({1, 1}, 4.0f);

  EXPECT_FLOAT_EQ(tensor.getValue({0, 0}), 1.0f);
  EXPECT_FLOAT_EQ(tensor.getValue({0, 1}), 2.0f);
  EXPECT_FLOAT_EQ(tensor.getValue({1, 0}), 3.0f);
  EXPECT_FLOAT_EQ(tensor.getValue({1, 1}), 4.0f);
  */
}

TEST_F(TensorTest, InvalidIndexTest) {
  // Tensor tensor({2, 2});
  // EXPECT_THROW(tensor.getValue({2, 0}), std::out_of_range);
  // EXPECT_THROW(tensor.setValue({0, 2}, 1.0f), std::out_of_range);
}

TEST_F(TensorTest, ReshapeTest) {
  // Tensor tensor({2, 3});
  // tensor.reshape({3, 2});
  // EXPECT_EQ(tensor.dimensions().size(), 2);
  // EXPECT_EQ(tensor.dimensions()[0], 3);
  // EXPECT_EQ(tensor.dimensions()[1], 2);
}

TEST_F(TensorTest, InvalidReshapeTest) {
  // Tensor tensor({2, 3});
  // EXPECT_THROW(tensor.reshape({2, 2}), std::invalid_argument);
}

TEST(TensorTest, BasicTest) {
  // Tensor tensor(10);
  // EXPECT_EQ(tensor.size(), 10);
  // EXPECT_NE(tensor.data(), nullptr);
}

}  // namespace test
}  // namespace nova_llm