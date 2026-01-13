#pragma once

#include "EdgeHermes/utils/macros.h"

namespace edgehermes {

class edgehermes_API Model {
 public:
  Model() = default;
  virtual ~Model() = default;

  virtual bool init() = 0;
  virtual bool load(const std::string& path) = 0;
  virtual bool unload() = 0;
};

using ModelPtr = std::shared_ptr<Model>;

}  // namespace edgehermes



