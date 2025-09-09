#pragma once

#if ENABLE_CUDA

#if defined(__CUDACC__) || defined(__CUDA__)
#include <cuda.h>
#include <cuda_runtime.h>
#else
#error "This file must be compiled with nvcc"
#endif

#endif