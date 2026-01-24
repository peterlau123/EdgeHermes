#pragma once
#include <cstddef>
#include <cstdint>

#include "EdgeHermes/common/device.h"

namespace edgehermes {

struct Buffer {
  uint8_t* data {nullptr};
  size_t size = 0;  // in bytes
  DeviceType device_type = DeviceType::CPU;
};


}  // namespace edgehermes




