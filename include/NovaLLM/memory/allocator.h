#pragma once

#include "NovaLLM/common/device.h"
#include "NovaLLM/utils/template.h"

namespace nova_llm {

class NOVA_LLM_API IAllocator {
 public:
  virtual ~IAllocator() = default;
  virtual void* allocate(size_t size) = 0;
  virtual void deallocate(void* ptr) = 0;
};

DEFINE_SHARED_PTR(IAllocator);

template <typename Derived>
class NOVA_LLM_API Allocator : public IAllocator {
 public:
  Allocator() = default;
  virtual ~Allocator() = default;

  void* allocate(size_t size) override {
    // 使用派生类的实现
    return static_cast<Derived*>(this)->do_allocate(size);
  }

  void deallocate(void* ptr) override {
    // 使用派生类的实现
    static_cast<Derived*>(this)->do_deallocate(ptr);
  }
};

// CPUAllocator 现在只需要实现 do_allocate 和 do_deallocate
class CPUAllocator : public Allocator<CPUAllocator> {
 public:
  CPUAllocator();
  ~CPUAllocator();

  void* do_allocate(size_t size);

  void do_deallocate(void* ptr);
};

class GPUAllocator : public Allocator<GPUAllocator> {
 public:
  GPUAllocator();
  ~GPUAllocator();

  void* do_allocate(size_t size);

  void do_deallocate(void* ptr);
};


}  // namespace nova_llm