#include "Peregrine/memory/arena.h"
#include "Peregrine/memory/allocator.h"

#include <gtest/gtest.h>
#include <memory>
#include <thread>

using namespace peregrine::amp;

class ArenaTest : public ::testing::Test {
 protected:
  void SetUp() override {
    size_class_system_ = &GetSizeClassSystem();
    config_.thread_cache_size_kb = 512;
  }

  void TearDown() override {}

  const SizeClassSystem* size_class_system_;
  AMPConfig config_;

  // Create allocator on demand to avoid unique_ptr copy issues
  IMemoryAllocatorPtr CreateAllocator() {
    return AllocatorFactory::Create(AllocatorType::STANDARD);
  }
};

// Test CPUArena construction and basic functionality
TEST_F(ArenaTest, CPUArenaConstruction) {
  EXPECT_NO_THROW({
    CPUArena arena(config_, CreateAllocator(), true);  // With NUMA
  });

  EXPECT_NO_THROW({
    CPUArena arena(config_, CreateAllocator(), false);  // Without NUMA
  });
}

// Test CPUArena device type
TEST_F(ArenaTest, CPUArenaDeviceType) {
  CPUArena arena(config_, CreateAllocator());
  EXPECT_EQ(arena.GetDeviceType(), DeviceType::CPU);
}

// Test CPUArena basic allocation
TEST_F(ArenaTest, CPUArenaAllocateBasic) {
  CPUArena arena(config_, CreateAllocator());

  void* ptr = arena.Allocate(128);
  EXPECT_NE(ptr, nullptr);

  // Should be able to deallocate
  arena.Deallocate(ptr, 128);
}

// Test CPUArena allocate zero size
TEST_F(ArenaTest, CPUArenaAllocateZero) {
  CPUArena arena(config_, CreateAllocator());

  void* ptr = arena.Allocate(0);
  EXPECT_EQ(ptr, nullptr);
}

// Test CPUArena aligned allocation
TEST_F(ArenaTest, CPUArenaAllocateAligned) {
  CPUArena arena(config_, CreateAllocator());

  void* ptr = arena.AllocateAligned(128, 64);
  EXPECT_NE(ptr, nullptr);

  // Check alignment
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0);

  arena.Deallocate(ptr, 128);
}

// Test CPUArena statistics
TEST_F(ArenaTest, CPUArenaStats) {
  CPUArena arena(config_, CreateAllocator());

  auto initial_stats = arena.GetStats();
  EXPECT_GE(initial_stats.total_allocated, 0);

  // Allocate some memory
  void* ptr1 = arena.Allocate(256);
  void* ptr2 = arena.Allocate(512);

  auto after_alloc_stats = arena.GetStats();
  EXPECT_GE(after_alloc_stats.total_allocated, initial_stats.total_allocated);

  // Deallocate
  arena.Deallocate(ptr1, 256);
  arena.Deallocate(ptr2, 512);

  auto final_stats = arena.GetStats();
  EXPECT_GE(final_stats.total_allocated, 0);
}

// Test CPUArena health check
TEST_F(ArenaTest, CPUArenaHealth) {
  CPUArena arena(config_, CreateAllocator());
  EXPECT_TRUE(arena.IsHealthy());
}

// Test CPUArena destructor
TEST_F(ArenaTest, CPUArenaDestructor) {
  {
    CPUArena arena(config_, CreateAllocator());

    // Allocate some memory and let it go out of scope
    void* ptr = arena.Allocate(128);
    EXPECT_NE(ptr, nullptr);
    // Don't deallocate - destructor should handle cleanup
  }
  // Should not crash
  SUCCEED();
}

// Test GPUArena (currently a stub)
TEST_F(ArenaTest, GPUArenaConstruction) {
  EXPECT_NO_THROW({
    GPUArena arena(config_, CreateAllocator(), true);  // With CUDA managed
  });

  EXPECT_NO_THROW({
    GPUArena arena(config_, CreateAllocator(), false);  // Without CUDA managed
  });
}

// Test GPUArena device type
TEST_F(ArenaTest, GPUArenaDeviceType) {
  GPUArena arena(config_, CreateAllocator());
  EXPECT_EQ(arena.GetDeviceType(), DeviceType::CUDA);
}

// Test GPUArena allocation (should return nullptr for now)
TEST_F(ArenaTest, GPUArenaAllocate) {
  GPUArena arena(config_, CreateAllocator());

  void* ptr = arena.Allocate(128);
  // GPU arena is not implemented yet, should return nullptr
  EXPECT_EQ(ptr, nullptr);

  // Deallocate should not crash even with nullptr
  arena.Deallocate(nullptr, 128);
}

// Test GPUArena health (should be unhealthy since not implemented)
TEST_F(ArenaTest, GPUArenaHealth) {
  GPUArena arena(config_, CreateAllocator());
  EXPECT_FALSE(arena.IsHealthy());
}

// Test ArenaRouter construction
TEST_F(ArenaTest, ArenaRouterConstruction) {
  EXPECT_NO_THROW({
    ArenaRouter router(config_);
  });
}

// Test ArenaRouter with arenas
TEST_F(ArenaTest, ArenaRouterWithCPUArena) {
  ArenaRouter router(config_);

  // Initialize with CPU arena
  router.InitializeArenas(CreateAllocator());

  // Should be able to get CPU arena
  IArena* cpu_arena = router.GetArena(DeviceType::CPU);
  EXPECT_NE(cpu_arena, nullptr);
  EXPECT_EQ(cpu_arena->GetDeviceType(), DeviceType::CPU);

  // Should not have GPU arena
  IArena* gpu_arena = router.GetArena(DeviceType::CUDA);
  EXPECT_EQ(gpu_arena, nullptr);
}

// Test ArenaRouter allocation through router
TEST_F(ArenaTest, ArenaRouterAllocate) {
  ArenaRouter router(config_);
  router.InitializeArenas(CreateAllocator());

  void* ptr = router.Allocate(256, DeviceType::CPU);
  EXPECT_NE(ptr, nullptr);

  router.Deallocate(ptr, 256, DeviceType::CPU);
}

// Test ArenaRouter global stats
TEST_F(ArenaTest, ArenaRouterStats) {
  ArenaRouter router(config_);
  router.InitializeArenas(CreateAllocator());

  auto stats = router.GetGlobalStats();
  EXPECT_GE(stats.total_allocated, 0);
}

// Test ArenaRouter health
TEST_F(ArenaTest, ArenaRouterHealth) {
  ArenaRouter router(config_);
  router.InitializeArenas(CreateAllocator());

  EXPECT_TRUE(router.AreAllArenasHealthy());
}

// Test ArenaRouter without initialization
TEST_F(ArenaTest, ArenaRouterNotInitialized) {
  ArenaRouter router(config_);

  // Should return nullptr for uninitialized arenas
  IArena* arena = router.GetArena(DeviceType::CPU);
  EXPECT_EQ(arena, nullptr);

  // Allocate should return nullptr
  void* ptr = router.Allocate(128, DeviceType::CPU);
  EXPECT_EQ(ptr, nullptr);

  // Stats should still work (empty)
  auto stats = router.GetGlobalStats();
  EXPECT_GE(stats.total_allocated, 0);
}

// Test multiple size allocations through arenas
TEST_F(ArenaTest, MultipleSizeAllocations) {
  CPUArena arena(config_, CreateAllocator());

  std::vector<size_t> sizes = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};

  std::vector<void*> allocations;

  // Allocate different sizes
  for (size_t size : sizes) {
    void* ptr = arena.Allocate(size);
    EXPECT_NE(ptr, nullptr);
    allocations.push_back(ptr);
  }

  // Deallocate in reverse order
  for (auto it = allocations.rbegin(); it != allocations.rend(); ++it) {
    arena.Deallocate(*it, sizes[allocations.rend() - it - 1]);
  }
}

// Test arena interface polymorphism
TEST_F(ArenaTest, InterfacePolymorphism) {
  CPUArena cpu_arena(config_, CreateAllocator());
  GPUArena gpu_arena(config_, CreateAllocator());

  // Both should implement IArena
  IArena* cpu_interface = &cpu_arena;
  IArena* gpu_interface = &gpu_arena;

  EXPECT_EQ(cpu_interface->GetDeviceType(), DeviceType::CPU);
  EXPECT_EQ(gpu_interface->GetDeviceType(), DeviceType::CUDA);

  // Test virtual function calls
  void* cpu_ptr = cpu_interface->Allocate(64);
  EXPECT_NE(cpu_ptr, nullptr);
  cpu_interface->Deallocate(cpu_ptr, 64);

  void* gpu_ptr = gpu_interface->Allocate(64);
  EXPECT_EQ(gpu_ptr, nullptr);  // GPU not implemented
  gpu_interface->Deallocate(gpu_ptr, 64);
}

// Test arena configuration variations
TEST_F(ArenaTest, ConfigurationVariations) {
  std::vector<size_t> cache_sizes = {0, 64, 512, 2048};

  for (size_t cache_size : cache_sizes) {
    AMPConfig test_config = config_;
    test_config.thread_cache_size_kb = cache_size;

    CPUArena arena(test_config, CreateAllocator());

    // Test basic functionality
    void* ptr = arena.Allocate(128);
    EXPECT_NE(ptr, nullptr);
    arena.Deallocate(ptr, 128);
  }
}

// Test concurrent arena access (basic smoke test)
TEST_F(ArenaTest, ConcurrentArenaAccess) {
  CPUArena arena(config_, CreateAllocator());

  const int num_threads = 4;
  const int operations_per_thread = 25;

  auto thread_func = [&arena]() {
    for (int i = 0; i < operations_per_thread; ++i) {
      void* ptr = arena.Allocate(64);
      if (ptr != nullptr) {
        // Quick write to ensure memory is valid
        memset(ptr, 0xBB, 64);
        arena.Deallocate(ptr, 64);
      }
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(thread_func);
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

// Test arena edge cases
TEST_F(ArenaTest, ArenaEdgeCases) {
  CPUArena arena(config_, CreateAllocator());

  // Very small allocation
  void* tiny = arena.Allocate(1);
  EXPECT_NE(tiny, nullptr);
  arena.Deallocate(tiny, 1);

  // Large allocation (may use different code path)
  void* large = arena.Allocate(1024 * 1024);  // 1MB
  if (large != nullptr) {
    arena.Deallocate(large, 1024 * 1024);
  }

  // Aligned allocation with various alignments
  std::vector<size_t> alignments = {1, 2, 4, 8, 16, 32, 64};
  for (size_t alignment : alignments) {
    void* aligned = arena.AllocateAligned(128, alignment);
    if (aligned != nullptr) {
      EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % alignment, 0);
      arena.Deallocate(aligned, 128);
    }
  }
}

// Test arena destructor with active allocations
TEST_F(ArenaTest, ArenaDestructorWithAllocations) {
  // Note: In a real implementation, this would be a memory leak test
  // For now, just ensure no crashes
  {
    CPUArena arena(config_, CreateAllocator());

    // Allocate but don't deallocate
    void* ptr1 = arena.Allocate(64);
    void* ptr2 = arena.Allocate(128);
    void* ptr3 = arena.Allocate(256);

    // Destructor should handle cleanup (though allocations may leak)
  }
  SUCCEED();
}




