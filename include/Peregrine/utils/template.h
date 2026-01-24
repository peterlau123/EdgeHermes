#pragma once
#include <memory>

template <typename T>
struct SharedPtrWrapper {
  using Type = std::shared_ptr<T>;
};

#define DEFINE_SHARED_PTR(X) using X##SharedPtr = SharedPtrWrapper<X>::Type

template <typename T>
struct UniquePtrWrapper {
  using Type = std::unique_ptr<T>;
};

#define DEFINE_UNIQUE_PTR(X) using X##UniquePtr = UniquePtrWrapper<X>::Type




