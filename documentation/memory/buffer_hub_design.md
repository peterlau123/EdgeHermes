# NovaLLM Memory Management System Redesign

## 1. Executive Summary

This document proposes a redesign of the NovaLLM memory management system, migrating from the current Segregated Free List (BufferHub) approach to an Adaptive Memory Pool (AMP) system with pluggable third-party allocators integration.

**Goal**: Improve performance, scalability, and maintainability while enabling integration of high-performance allocators like tcmalloc, jemalloc, and mimalloc.

## 2. Current Design Analysis

### Current Architecture Overview
- **BufferHub**: Segregated free lists with fixed size classes (64B → 4KB → 128MB → 4GB)
- **BufferManager**: Singleton manager for CPU/GPU buffer hubs with basic thread safety
- **Allocators**: Simple CPU/GPU allocators using std::malloc/cstdlib

### Current Strengths
- Thread-safe segregated lists
- Clean device abstraction
- Memory pool prevents fragmentation

### Current Weaknesses
- Fixed size classes limit flexibility
- No coalescing between size classes
- Single mutex limits concurrency
- Hard to integrate third-party allocators
- Singleton pattern reduces testability

## 3. Proposed Adaptive Memory Pool (AMP) Architecture

### 3.1 High-Level Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                   Adaptive Memory Pool System                    │
├──────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐  │
│  │ Thread Cache│  │ Central    │  │   Page     │  │  Stats   │  │
│  │             │  │ Cache      │  │   Heap     │  │ Monitor  │  │
│  │ Lock-free   │  │ Shared     │  │ Fallback   │  │          │  │
│  │ Small Allocs│  │ Lists      │  │ Allocator  │  │ Perf     │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │ Metrics │  │
├─────────────────────────────────────────────────────┼──────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ ∟          │
│  │ CPU Arena   │  │ GPU Arena  │  │Arena Router│            │
│  │ (NUMA-aware)│  │(CUDA-aware)│  │             │            │
│  └─────────────┘  └─────────────┘  └─────────────┘            │
├──────────────────────────────────────────────────────────────────┤
│         Pluggable Allocators: tcmalloc | jemalloc | mimalloc    │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 Core Components

#### Thread Cache (Lock-Free)
- **Purpose**: Fast, per-thread allocation for small objects
- **Implementation**: Lock-free data structures (atomic operations)
- **Capacity**: Limited cache size per thread (512KB default)

#### Central Cache (Low-Contention)
- **Purpose**: Shared free lists for size classes
- **Implementation**: Fine-grained locking per size class
- **Features**: Batch allocation from page heap

#### Page Heap (Large Allocations)
- **Purpose**: Handles large allocations and fallback
- **Implementation**: Delegates to underlying allocator system

#### Size Class System (Adaptive)
- **Purpose**: Maps allocation sizes to efficient classes
- **Improvements**: Dynamic size class optimization based on usage patterns

## 4. Implementation Plan (8-Week Roadmap)

### Phase 1: Core Infrastructure (Week 1-2)

**Deliverables:**
- Define `IMemoryAllocator` interface
- Implement basic `SizeClassSystem`
- Create `ThreadCache` with lock-free operations

**Key Files:**
```cpp
// include/memory/amp_system.h
class IMemoryAllocator {
    virtual void* Allocate(size_t size) = 0;
    virtual void Deallocate(void* ptr) = 0;
    virtual void* AllocateAligned(size_t size, size_t alignment) = 0;
};

// include/memory/size_class.h
class SizeClassSystem {
    static constexpr size_t NUM_SIZE_CLASSES = 128;
    size_t GetSizeClass(size_t size);
    size_t GetClassMaxSize(size_t class_id);
};
```

### Phase 2: Central Cache & Page Heap (Week 3-4)

**Deliverables:**
- `CentralCache` with per-class locking
- `PageHeap` for large allocations
- Memory statistics collection

**Integration Points:**
- Replace `BufferHub::gradeLevel()` with adaptive sizing
- Maintain `Buffer` API compatibility

### Phase 3: Arena System (Week 5-6)

**Deliverables:**
- NUMA-aware CPU arenas
- Device-specific GPU arenas
- Arena routing and management

**Migration Strategy:**
```cpp
class AMPBufferManager : public nova_llm::BufferManager {
private:
    // New internal implementation
    std::unique_ptr<AMP::Arena> arenas_[DeviceType::COUNT];
};

// Feature flag for gradual rollout
DEFINE_CONFIG_FLAG(use_amp_system, false);
```

### Phase 4: Third-Party Integration & Tuning (Week 7-8)

**Deliverables:**
- Wrappers for tcmalloc, jemalloc, mimalloc
- Performance tuning and benchmarks
- Production readiness validation

## 5. Third-Party Allocator Integration

### 5.1 Interface Design

```cpp
// include/memory/allocator_wrapper.h
class AllocatorWrapper : public IMemoryAllocator {
public:
    enum class Type { TCMALLOC, JEMALLOC, MIMALLOC, STANDARD };

    explicit AllocatorWrapper(Type type,
                             const std::unordered_map<std::string, std::string>& options = {});

    void* Allocate(size_t size) override;
    void Deallocate(void* ptr) override;
    void* AllocateAligned(size_t size, size_t alignment) override;

private:
    std::unique_ptr<IMemoryAllocator> impl_;
};
```

### 5.2 TCMalloc Integration

**Installation:**
```bash
# Ubuntu/Debian
apt-get install libgoogle-perftools-dev

# CMake integration
find_package(PkgConfig)
pkg_check_modules(TCMALLOC REQUIRED libtcmalloc)
target_link_libraries(novallm ${TCMALLOC_LIBRARIES})
```

**Wrapper Implementation:**
```cpp
class TCMallocWrapper : public IMemoryAllocator {
public:
    void* Allocate(size_t size) override {
        return tc_malloc(size);
    }

    void Deallocate(void* ptr) override {
        tc_free(ptr);
    }

    void* AllocateAligned(size_t size, size_t alignment) override {
        return tc_memalign(alignment, size);
    }
};
```

### 5.3 Jemalloc Integration

**Installation:**
```bash
# Ubuntu
apt-get install libjemalloc-dev

# macOS
brew install jemalloc

# CMake
find_library(JEMALLOC_LIBRARY jemalloc)
target_link_libraries(novallm ${JEMALLOC_LIBRARY})
```

### 5.4 Mimalloc Integration

**Installation:**
```cmake
# CMakeLists.txt
add_subdirectory(external/mimalloc)
target_link_libraries(novallm mimalloc)
```

**Header-Only Usage:**
```cpp
#define MI_MALLOC_OVERRIDE
#include <mimalloc.h>
```

### 5.5 Configuration System

```yaml
# memory_config.yaml
memory:
  allocator_type: "tcmalloc"  # Options: tcmalloc, jemalloc, mimalloc, standard

  tcmalloc_options:
    narenas: 4                     # Number of arenas
    dirty_decay_ms: 10000         # Dirty page decay time
    muzzy_decay_ms: 5000          # Muzzy page decay time

  jemalloc_options:
    narenas: 4
    dirty_decay_ms: 10000
    muzzy_decay_ms: 5000

  performance:
    thread_cache_size_mb: 2       # Per-thread cache size
    central_cache_limit_mb: 128   # Central cache size limit

  monitoring:
    enable_stats: true
    sample_rate: 0.01             # Sample 1% of allocations for profiling

# CPU-specific settings
cpu:
  numa_aware: true                # Use NUMA-aware allocation
  max_cache_threads: 64           # Max threads with caches

# GPU-specific settings
gpu:
  cuda_managed_memory: false       # Use CUDA managed memory
  preallocate_limit_gb: 1         # Pre-allocate limit per device
```

**Runtime Initialization:**
```cpp
void initialize_memory_system() {
    MemoryConfig config;
    config.load_from_file("memory_config.yaml");

    auto allocator = AllocatorFactory::create(config.allocator_type, config.options);
    AMPSystem::initialize(std::move(allocator), config.performance);
}
```

## 6. API Compatibility & Migration

### 6.1 Maintain Current APIs

```cpp
// Existing BufferManager API remains unchanged for clients
class BufferManager {
public:
    static BufferManager& getInstance();  // Still works
    Buffer fetch(size_t size, DeviceType device);
    void put(Buffer& buffer);
    // ... existing methods
};

// Internal implementation changes
namespace AMP {
    class System {
        static BufferManager& getInstance() {
            static AMPBufferManager instance;
            return instance;
        }
    };
}
```

### 6.2 Feature Toggles

```cpp
// Runtime feature flags
DEFINE_CONFIG_FLAG(use_amp_system, false);
DEFINE_CONFIG_FLAG(allocator_type, "standard");  // tcmalloc, jemalloc, etc.

// Conditional compilation
#ifdef USE_AMP_SYSTEM
    using BufferManager = AMP::BufferManager;
#else
    using BufferManager = Legacy::BufferManager;
#endif
```

## 7. Performance Expectations

### 7.1 Performance Targets

| Metric | Current | Target | Expected Improvement |
|--------|---------|--------|---------------------|
| Small allocation latency | ~50ns | <20ns | 2.5x faster |
| Medium allocation latency | ~200ns | ~100ns | 2x faster |
| Large allocation latency | ~10μs | ~5μs | 2x faster |
| Memory fragmentation | 25-35% | <15% | 50% reduction |
| Thread scaling efficiency | 60% | >85% | 40% improvement |
| Peak memory efficiency | 85% | >95% | 11% improvement |

### 7.2 Benchmark Requirements

**Small Object Benchmark:**
```cpp
// Allocate/deallocate 8-128 byte objects
// Measure: latency, throughput, fragmentation
for (size_t size : {8, 16, 32, 64, 128}) {
    benchmark_size_class(size, 1000000 /* iterations */);
}
```

**Concurrent Allocation Benchmark:**
```cpp
// Multiple threads simultaneously allocating
// Measure: lock contention, scaling efficiency
std::vector<std::thread> threads;
for (int t = 0; t < std::thread::hardware_concurrency(); ++t) {
    threads.emplace_back(concurrent_allocation_test);
}
```

### 7.3 Memory Usage Monitoring

```cpp
struct MemoryStats {
    size_t total_allocated;
    size_t active_allocations;
    double fragmentation_ratio;
    std::unordered_map<size_t, size_t> size_class_usage;

    // Per-thread cache statistics
    struct ThreadStats {
        size_t hits;
        size_t misses;
        size_t cache_size;
    };
    std::vector<ThreadStats> thread_stats;
};
```

## 8. Risk Assessment & Mitigation

### 8.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Performance regression | Medium | High | Comprehensive benchmarking, fallback mechanism |
| Memory leaks/corruption | Low | High | Valgrind testing, automated leak detection |
| Third-party dependencies | Low | Medium | Vendor-neutral interface, local copies if needed |
| Increased complexity | Medium | Medium | Modular design, extensive documentation |

### 8.2 Migration Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| API breaking changes | Low | Medium | Compatibility layer, gradual rollout |
| Integration bugs | Medium | High | Feature flags, staged deployment |
| Vendor lock-in | Low | Low | Pluggable architecture, multiple implementations |

### 8.3 Technical Debt Considerations

- **Interface Stability**: Maintain backwards compatibility for 6-12 months
- **Profiling Tools**: Build performance monitoring from day one
- **Documentation**: Comprehensive API documentation with examples
- **Testing**: 90%+ code coverage target

## 9. Implementation Quality Requirements

### 9.1 Code Quality Standards

- **Thread Safety**: All public APIs must be thread-safe unless explicitly documented otherwise
- **Error Handling**: Use exceptions for allocation failures, provide noexcept alternatives
- **Resource Management**: RAII for all resources, no manual cleanup required
- **Performance**: Zero-overhead abstractions, no virtual function calls in hot paths

### 9.2 Testing Requirements

- **Unit Tests**: 100% coverage for core components (size classes, thread cache)
- **Integration Tests**: End-to-end allocation patterns matching real workloads
- **Concurrency Tests**: ThreadSanitizer clean, stress tests with 100+ threads
- **Performance Tests**: Regression testing, baseline performance requirements

### 9.3 Documentation Requirements

- **Architecture Decision Records (ADRs)** for all major design decisions
- **API Reference Documentation** with examples for all public interfaces
- **Performance Tuning Guide** for system administrators
- **Migration Guide** with before/after code examples

## 10. Success Criteria

### 10.1 Functional Success
- [ ] All existing tests pass (API compatibility maintained)
- [ ] All new components have 90%+ test coverage
- [ ] Third-party allocator integration tested with all supported allocators
- [ ] NUMA-aware allocation verified on multi-socket systems

### 10.2 Performance Success
- [ ] Small object allocation < 20ns average latency
- [ ] >85% thread scaling efficiency at hardware concurrency
- [ ] <15% memory fragmentation in typical workloads
- [ ] No performance regressions vs current system

### 10.3 Quality Success
- [ ] Zero memory leaks detected in release builds
- [ ] Clean ThreadSanitizer and AddressSanitizer reports
- [ ] Documentation reviewed and approved by architecture team
- [ ] Production deployment approved by SRE team

This redesign provides a modern, flexible memory management system that can evolve with NovaLLM's needs while maintaining compatibility and improving performance across all use cases.
