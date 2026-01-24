#pragma once

#include "Peregrine/common/device.h"
#include "Peregrine/data/tensor.h"
#include "Peregrine/memory/allocator.h"
#include "Peregrine/model/model.h"
#include "Peregrine/utils/macros.h"

namespace peregrine {

class Impl;

class PEREGRINE_API Engine {
 public:
  Engine() = default;

  ~Engine();

  bool init();
  void run();

 private:
  Impl* impl_;
};


}  // namespace peregrine




