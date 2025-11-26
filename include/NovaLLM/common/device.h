#pragma once

#include "NovaLLM/utils/macros.h"

namespace nova_llm {

enum class DeviceType : uint32_t { UNKNOWN = 0, CPU = 0x01, CUDA = 0x02, METAL = 0x04 };

struct DeviceTypeFlags {
 public:
   [[nodiscard]] NOVA_LLM_API bool has(DeviceType type) const;

  NOVA_LLM_API void set(DeviceType type);

  NOVA_LLM_API void clear(DeviceType type);

  [[nodiscard]] NOVA_LLM_API constexpr DeviceType get() const;

 private:
  uint32_t flags_ = 0;
};

}  // namespace nova_llm
