#include <gtest/gtest.h>

#include "NovaLLM/NovaLLM-cpp.h"

using namespace nova_llm;

// Test fixture for EngineImpl
class EngineImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup code that will be called before each test
  }

  void TearDown() override {
    // Cleanup code that will be called after each test
  }
};

// Test initialization
TEST_F(EngineImplTest, InitializationTest) {
  auto engine = std::make_unique<Engine>();
  EXPECT_TRUE(engine->init());
}

// Main function to run all tests
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}