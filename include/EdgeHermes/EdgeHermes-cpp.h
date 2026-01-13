#pragma once

#include "EdgeHermes/common/device.h"
#include "EdgeHermes/data/tensor.h"
#include "EdgeHermes/memory/allocator.h"
#include "EdgeHermes/model/model.h"
#include "EdgeHermes/utils/macros.h"

namespace edgehermes {

class Impl;

class edgehermes_API Engine {
 public:
  Engine() = default;

  ~Engine();

  bool init();
  void run();

 private:
  Impl* impl_;
};


}  // namespace edgehermes



