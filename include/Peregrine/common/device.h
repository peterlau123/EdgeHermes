#pragma once

#include "Peregrine/utils/macros.h"

namespace peregrine {

enum class DeviceType : uint32_t { UNKNOWN = 0, CPU = 0x01, CUDA = 0x02, METAL = 0x04 };

struct DeviceTypeFlags {
 public:
   [[nodiscard]] PEREGRINE_API bool has(DeviceType type) const;

  PEREGRINE_API void set(DeviceType type);

  PEREGRINE_API void clear(DeviceType type);

  [[nodiscard]] PEREGRINE_API constexpr DeviceType get() const;

 private:
  uint32_t flags_ = 0;
};

}  // namespace peregrine




