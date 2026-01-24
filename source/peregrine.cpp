#include <fmt/format.h>

#include "Peregrine/Peregrine-cpp.h"
#include "Peregrine/utils/log.h"
#include "engine/EngineImpl.h"

namespace peregrine {

class Impl {
 public:
  [[nodiscard]] static Impl* build() {
    auto ptr = new Impl;
    return ptr;
  }

  static void destroy(Impl** ptr) {
    if (*ptr) {
      delete *ptr;
      *ptr = nullptr;
    }
  }

  EngineImplPtr engine_ptr;
};

bool Engine::init() {
  // Implementation
  return true;
}

void Engine::run() {}

Engine::~Engine() { Impl::destroy(&impl_); }

}  // namespace peregrine




