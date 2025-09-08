#pragma once

#include "NovaLLM/common/device.h"
#include "NovaLLM/memory/allocator.h"


namespace nova_llm {

class Env{
public:
    struct Config{
        // Configuration options for the environment
        DeviceTypeFlags device_flags;
        IAllocatorSharedPtr cpu_allocator{nullptr};
        IAllocatorSharedPtr gpu_allocator{nullptr};
        // Add more configuration options as needed
    };

    static void setup(const Config& config);

    static void teardown();
};

}