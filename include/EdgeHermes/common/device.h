#pragma once

#include "EdgeHermes/utils/macros.h"

namespace edgehermes {

enum class DeviceType : uint32_t { UNKNOWN = 0, CPU = 0x01, CUDA = 0x02, METAL = 0x04 };

struct DeviceTypeFlags {
 public:
   [[nodiscard]] edgehermes_API bool has(DeviceType type) const;

  edgehermes_API void set(DeviceType type);

  edgehermes_API void clear(DeviceType type);

  [[nodiscard]] edgehermes_API constexpr DeviceType get() const;

 private:
  uint32_t flags_ = 0;
};

}  // namespace edgehermes



