#include <fmt/format.h>

#include "EdgeHermes/EdgeHermes-cpp.h"
#include "EdgeHermes/utils/log.h"
#include "engine/EngineImpl.h"

namespace edgehermes {

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

}  // namespace edgehermes



