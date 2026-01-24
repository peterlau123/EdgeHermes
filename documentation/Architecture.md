# Peregrine System Architecture (系统架构�?

## Complete System Overview

```mermaid
graph TB
    %% External Users and Applications
    subgraph "👥 External Users<br/>外部用户"
        USER[End Users<br/>终端用户]
        DEV[Developers<br/>开发者]
        SYS[Systems<br/>系统集成]
    end

    %% Applications and APIs
    subgraph "📱 Application Layer<br/>应用�?
        APP[User Applications<br/>用户应用<br/>Chatbots, Tools, APIs]
        HTTP_API[HTTP API<br/>REST/gRPC]
        SDK[SDK & Libraries<br/>开发工具包]
    end

    %% Core Peregrine System
    subgraph "🧠 Peregrine Core<br/>Peregrine核心"
        ENGINE[LLM Engine<br/>LLM引擎<br/>Inference Pipeline]

        subgraph "⚙️ Engine Components<br/>引擎组件"
            TOKENIZER[Tokenizer<br/>分词器]
            MODEL_EXEC[Model Executor<br/>模型执行器]
            KV_CACHE[KV Cache<br/>键值缓存]
            SAMPLER[Sampler<br/>采样器]
        end

        subgraph "🏗�?Core Abstractions<br/>核心抽象"
            TENSOR_SYSTEM[Tensor System<br/>张量系统]
            BUFFER_MGR[Buffer Manager<br/>缓冲区管理器]
            DEVICE_ABS[Device Abstraction<br/>设备抽象]
        end
    end

    %% Memory Management System
    subgraph "💾 Advanced Memory Pool (AMP)<br/>高级内存�?
        AMP_CORE[AMP Core<br/>AMP核心]

        subgraph "🏛�?Memory Infrastructure<br/>内存基础设施"
            ARENA_ROUTER[Arena Router<br/>竞技场路由器<br/>CPU/GPU/NPU]
            THREAD_CACHE[Thread Cache<br/>线程缓存<br/>Per-thread Pools]
            CENTRAL_CACHE[Central Cache<br/>中央缓存<br/>Shared Free Lists]
            PAGE_HEAP[Page Heap<br/>页面�?br/>Large Allocations]
        end

        subgraph "🔧 Memory Allocators<br/>内存分配�?
            CPU_ALLOC[CPU Allocators<br/>CPU分配�?br/>TCMalloc, Jemalloc, Mimalloc]
            GPU_ALLOC[GPU Allocators<br/>GPU分配�?br/>CUDA, Managed Memory]
            NPU_ALLOC[NPU Allocators<br/>NPU分配�?br/>Future Support]
        end
    end

    %% Build and Development Tools
    subgraph "🔨 Build System<br/>构建系统"
        CMAKE[CMake<br/>构建配置]
        CONAN[Conan<br/>依赖管理<br/>Third-party Libraries]

        subgraph "📦 Dependencies<br/>依赖�?
            FMT[fmt<br/>格式化库]
            SPDLOG[spdlog<br/>日志库]
            GTEST[gtest<br/>测试框架]
            TCMALLOC_DEPS[TCMalloc<br/>高性能分配器]
            CUDA_DEPS[CUDA SDK<br/>GPU开发包]
        end
    end

    %% Testing and Quality Assurance
    subgraph "🧪 Testing & QA<br/>测试与质量保�?
        UNIT_TESTS[Unit Tests<br/>单元测试<br/>Allocator, Buffer, Tensor]
        INTEGRATION[Integration Tests<br/>集成测试<br/>End-to-end Pipelines]
        PERF_TESTS[Performance Tests<br/>性能测试<br/>Benchmarking]
        MEMORY_TESTS[Memory Tests<br/>内存测试<br/>Leak Detection]
    end

    %% CI/CD and Deployment
    subgraph "🚀 CI/CD & Deployment<br/>持续集成与部�?
        GITHUB_ACTIONS[GitHub Actions<br/>自动化流水线]
        BUILD_MATRIX[Build Matrix<br/>构建矩阵<br/>Multi-platform]
        RELEASE[Release Management<br/>版本管理<br/>Binaries, Packages]
    end

    %% Documentation and Community
    subgraph "📚 Documentation & Community<br/>文档与社�?
        DOCS[Technical Docs<br/>技术文�?br/>API, Architecture]
        EXAMPLES[Code Examples<br/>代码示例<br/>Tutorials, Demos]
        COMMUNITY[Community<br/>社区<br/>Issues, Discussions]
    end

    %% Data Flow and Connections
    USER --> APP
    DEV --> SDK
    SYS --> HTTP_API

    APP --> HTTP_API
    HTTP_API --> ENGINE
    SDK --> ENGINE

    ENGINE --> TOKENIZER
    TOKENIZER --> MODEL_EXEC
    MODEL_EXEC --> KV_CACHE
    KV_CACHE --> SAMPLER

    ENGINE --> TENSOR_SYSTEM
    TENSOR_SYSTEM --> BUFFER_MGR
    BUFFER_MGR --> DEVICE_ABS

    TENSOR_SYSTEM --> AMP_CORE
    BUFFER_MGR --> AMP_CORE

    AMP_CORE --> ARENA_ROUTER
    ARENA_ROUTER --> THREAD_CACHE
    THREAD_CACHE --> CENTRAL_CACHE
    CENTRAL_CACHE --> PAGE_HEAP

    ARENA_ROUTER --> CPU_ALLOC
    ARENA_ROUTER --> GPU_ALLOC
    ARENA_ROUTER --> NPU_ALLOC

    CMAKE --> CONAN
    CONAN --> FMT
    CONAN --> SPDLOG
    CONAN --> GTEST
    CONAN --> TCMALLOC_DEPS
    CONAN --> CUDA_DEPS

    UNIT_TESTS --> ENGINE
    INTEGRATION --> ENGINE
    PERF_TESTS --> ENGINE
    MEMORY_TESTS --> AMP_CORE

    CMAKE --> GITHUB_ACTIONS
    GITHUB_ACTIONS --> BUILD_MATRIX
    BUILD_MATRIX --> RELEASE

    DOCS --> EXAMPLES
    EXAMPLES --> COMMUNITY

    %% Styling
    classDef external fill:#e8f4fd,stroke:#1976d2,stroke-width:2px
    classDef application fill:#e3f2fd,stroke:#1976d2,stroke-width:2px
    classDef core fill:#e8f5e8,stroke:#388e3c,stroke-width:2px
    classDef memory fill:#fce4ec,stroke:#c2185b,stroke-width:2px
    classDef build fill:#fff3e0,stroke:#f57c00,stroke-width:2px
    classDef testing fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px
    classDef deployment fill:#e0f2f1,stroke:#00695c,stroke-width:2px
    classDef docs fill:#f5f5f5,stroke:#424242,stroke-width:2px

    class USER,DEV,SYS external
    class APP,HTTP_API,SDK application
    class ENGINE,TOKENIZER,MODEL_EXEC,KV_CACHE,SAMPLER,TENSOR_SYSTEM,BUFFER_MGR,DEVICE_ABS core
    class AMP_CORE,ARENA_ROUTER,THREAD_CACHE,CENTRAL_CACHE,PAGE_HEAP,CPU_ALLOC,GPU_ALLOC,NPU_ALLOC memory
    class CMAKE,CONAN,FMT,SPDLOG,GTEST,TCMALLOC_DEPS,CUDA_DEPS build
    class UNIT_TESTS,INTEGRATION,PERF_TESTS,MEMORY_TESTS testing
    class GITHUB_ACTIONS,BUILD_MATRIX,RELEASE deployment
    class DOCS,EXAMPLES,COMMUNITY docs
```

## System Components Overview

### 1. External Ecosystem (外部生�?
- **End Users**: Applications using Peregrine (chatbots, analysis tools)
- **Developers**: SDK users building applications
- **Systems**: Enterprise integrations via APIs

### 2. Application Layer (应用�?
- **User Applications**: Client applications built on Peregrine
- **HTTP API**: REST/gRPC interfaces for system integration
- **SDK & Libraries**: Development tools and language bindings

### 3. Peregrine Core (Peregrine核心)
- **LLM Engine**: Main inference pipeline orchestration
- **Engine Components**:
  - Tokenizer: Text processing and tokenization
  - Model Executor: Neural network execution
  - KV Cache: Attention mechanism optimization
  - Sampler: Output token generation
- **Core Abstractions**:
  - Tensor System: Multi-dimensional array operations
  - Buffer Manager: Memory buffer lifecycle
  - Device Abstraction: CPU/GPU/NPU unified interface

### 4. Advanced Memory Pool (AMP) (高级内存�?
- **AMP Core**: Memory management orchestration
- **Memory Infrastructure**:
  - Arena Router: Device-specific memory routing
  - Thread Cache: Per-thread memory pools
  - Central Cache: Shared free lists across threads
  - Page Heap: Large allocation handling
- **Memory Allocators**:
  - CPU Allocators: TCMalloc, Jemalloc, Mimalloc, Standard
  - GPU Allocators: CUDA, Managed Memory
  - NPU Allocators: Future neural processor support

### 5. Build System (构建系统)
- **CMake**: Build configuration and compilation
- **Conan**: Dependency management and package resolution
- **Dependencies**: All third-party libraries (fmt, spdlog, gtest, CUDA, etc.)

### 6. Testing & QA (测试与质量保�?
- **Unit Tests**: Component-level testing (allocators, buffers, tensors)
- **Integration Tests**: End-to-end pipeline testing
- **Performance Tests**: Benchmarking and optimization validation
- **Memory Tests**: Leak detection and memory correctness

### 7. CI/CD & Deployment (持续集成与部�?
- **GitHub Actions**: Automated build and test pipelines
- **Build Matrix**: Multi-platform compilation (Linux, macOS, Windows)
- **Release Management**: Binary distribution and packaging

### 8. Documentation & Community (文档与社�?
- **Technical Docs**: API documentation and architecture guides
- **Code Examples**: Tutorials and demonstration code
- **Community**: Issue tracking, discussions, and collaboration

## Key System Flows

### Inference Request Flow (推理请求流程)
```
User Request �?HTTP API �?LLM Engine �?Tokenizer �?Model Executor �?KV Cache �?Sampler �?Response
```

### Memory Allocation Flow (内存分配流程)
```
Tensor Creation �?Buffer Manager �?AMP Core �?Arena Router �?Thread Cache �?Central Cache �?Page Heap �?Hardware Allocator
```

### Development Flow (开发流�?
```
Code Changes �?GitHub Actions �?Build Matrix �?Unit Tests �?Integration Tests �?Performance Tests �?Release
```

## Design Principles (设计原则)

1. **Modularity**: Clear separation between components
2. **Extensibility**: Pluggable allocators and modular architecture
3. **Performance**: High-performance memory management and inference
4. **Reliability**: Comprehensive testing and error handling
5. **Developer Experience**: Rich tooling and documentation
6. **Cross-Platform**: Support for multiple operating systems and architectures

## Technology Stack (技术栈)

- **Core Language**: C++17 with modern idioms
- **Build System**: CMake with Conan dependency management
- **Memory Management**: Custom AMP system with multiple allocators
- **Testing**: Google Test framework
- **Documentation**: Markdown with Mermaid diagrams
- **CI/CD**: GitHub Actions with multi-platform support
- **GPU Support**: CUDA with fallback mechanisms




