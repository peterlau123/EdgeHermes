#include "EngineImpl.h"

#include <memory>

namespace edgehermes {

EngineImpl::EngineImplPtr EngineImpl::build() {

  auto ptr = std::make_shared<EngineImpl>();
  return ptr;
}

bool EngineImpl::init() {
  bool ret = false;

  return ret;
}

bool EngineImpl::parse(const std::string& modelPath) {
  bool ret {true};
  // map model file to cpu memory

  // load weights for tokenizer and model
  return ret;
}

std::string EngineImpl::chat(const std::string& prompt) {
  std::string ret;

  return ret;
}

}  // namespace edgehermes




