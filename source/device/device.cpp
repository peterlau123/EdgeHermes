#include "EdgeHermes/common/device.h"

namespace edgehermes {

bool DeviceTypeFlags::has(DeviceType type) const { return (flags_ & static_cast<uint32_t>(type)) != 0; }

// 添加设备
void DeviceTypeFlags::set(DeviceType type) { flags_ |= static_cast<uint32_t>(type); }

// 移除设备
void DeviceTypeFlags::clear(DeviceType type) { flags_ &= ~static_cast<uint32_t>(type); }

// 获取所有设�?
constexpr DeviceType DeviceTypeFlags::get() const { return static_cast<DeviceType>(flags_); }
}  // namespace edgehermes




