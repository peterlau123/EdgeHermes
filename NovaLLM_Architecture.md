# NovaLLM Architecture Overview

## Architecture Diagram

```mermaid
graph TB
    %% Application Layer
    subgraph "Application Layer"
        APP[Applications<br/>Built on Runtime]
        API[API Interface]
    end

    %% Engine Layer
    subgraph "Engine Layer"
        ENGINE[Engine<br/>LLM Processing Logic]

        subgraph "Engine Components"
            INPUT[input_processor<br/>Text/Token Processing]
            INFERENCE[inference<br/>Model Execution]
            OUTPUT[output_processor<br/>Result Formatting]
        end
    end

    %% LLM Inference Layer
    subgraph "LLM Inference Layer"
        INFERENCE_CORE[LLM Inference Core]

        subgraph "Model Layer"
            MODEL[Model<br/>Neural Network Architecture]
            LAYERS[Layers<br/>Attention, FeedForward, etc.]
            WEIGHTS[Weights & Biases<br/>Model Parameters]
        end
    end

    %% Base Abstraction Layer
    subgraph "Base Abstraction Layer"
        DATA_STRUCTS[Data Structures]

        subgraph "Core Data Types"
            TENSOR[Tensor<br/>Multi-dimensional Arrays]
            BUFFER[Buffer<br/>Memory Management]
            DEVICE[Device<br/>CPU/GPU/NPU Abstraction]
            DTYPE[DataType<br/>INT8, FLOAT32, etc.]
        end
    end

    %% Memory Layer
    subgraph "Memory Layer"
        MEMORY_MGR[Memory Management System]

        subgraph "CPU Memory"
            CPU_ALLOC[CPU Allocators]
            CPU_STANDARD[StandardAllocator<br/>malloc/free]
            CPU_TCMALLOC[TCMallocAllocator<br/>High-performance]
            CPU_JEMALLOC[JemallocAllocator<br/>Scalable]
            CPU_MIMALLOC[MimallocAllocator<br/>Modern]
        end

        subgraph "GPU Memory"
            GPU_ALLOC[GPU Allocators]
            GPU_CUDA[CUDAAllocator<br/>cudaMalloc/cudaFree]
            GPU_MANAGED[Managed Memory<br/>Unified Addressing]
            GPU_DEVICE[Device Memory<br/>GPU Exclusive]
        end

        subgraph "NPU Memory"
            NPU_ALLOC[NPU Allocators]
            NPU_SPECIFIC[NPU-specific<br/>Memory Management]
        end

        subgraph "Memory Infrastructure"
            AMP[AMP System<br/>Adaptive Memory Pool]
            ARENA_ROUTER[Arena Router<br/>Device Selection]
            THREAD_CACHE[Thread Cache<br/>Per-thread Pools]
            CENTRAL_CACHE[Central Cache<br/>Shared Free Lists]
            PAGE_HEAP[Page Heap<br/>Large Allocations]
        end
    end

    %% Data Flow Connections
    APP --> API
    API --> ENGINE

    ENGINE --> INPUT
    INPUT --> INFERENCE
    INFERENCE --> OUTPUT

    ENGINE --> INFERENCE_CORE
    INFERENCE_CORE --> MODEL
    MODEL --> LAYERS
    LAYERS --> WEIGHTS

    INFERENCE_CORE --> DATA_STRUCTS
    DATA_STRUCTS --> TENSOR
    DATA_STRUCTS --> BUFFER
    DATA_STRUCTS --> DEVICE
    DATA_STRUCTS --> DTYPE

    DATA_STRUCTS --> MEMORY_MGR

    %% Memory Layer Internal Connections
    MEMORY_MGR --> CPU_ALLOC
    MEMORY_MGR --> GPU_ALLOC
    MEMORY_MGR --> NPU_ALLOC

    CPU_ALLOC --> CPU_STANDARD
    CPU_ALLOC --> CPU_TCMALLOC
    CPU_ALLOC --> CPU_JEMALLOC
    CPU_ALLOC --> CPU_MIMALLOC

    GPU_ALLOC --> GPU_CUDA
    GPU_CUDA --> GPU_MANAGED
    GPU_CUDA --> GPU_DEVICE

    NPU_ALLOC --> NPU_SPECIFIC

    %% Infrastructure Connections
    MEMORY_MGR --> AMP
    AMP --> ARENA_ROUTER
    ARENA_ROUTER --> THREAD_CACHE
    THREAD_CACHE --> CENTRAL_CACHE
    CENTRAL_CACHE --> PAGE_HEAP

    %% Cross-layer Dependencies
    TENSOR -.->|uses| CPU_ALLOC
    TENSOR -.->|uses| GPU_ALLOC
    BUFFER -.->|uses| AMP
    MODEL -.->|uses| TENSOR
    LAYERS -.->|uses| BUFFER

    %% Styling
    classDef applicationLayer fill:#e1f5fe,stroke:#01579b,stroke-width:2px
    classDef engineLayer fill:#f3e5f5,stroke:#4a148c,stroke-width:2px
    classDef inferenceLayer fill:#e8f5e8,stroke:#1b5e20,stroke-width:2px
    classDef abstractionLayer fill:#fff3e0,stroke:#e65100,stroke-width:2px
    classDef memoryLayer fill:#fce4ec,stroke:#880e4f,stroke-width:2px
    classDef infrastructure fill:#f5f5f5,stroke:#424242,stroke-width:1px

    class APP,API applicationLayer
    class ENGINE,INPUT,INFERENCE,OUTPUT engineLayer
    class INFERENCE_CORE,MODEL,LAYERS,WEIGHTS inferenceLayer
    class DATA_STRUCTS,TENSOR,BUFFER,DEVICE,DTYPE abstractionLayer
    class MEMORY_MGR,CPU_ALLOC,GPU_ALLOC,NPU_ALLOC memoryLayer
    class AMP,ARENA_ROUTER,THREAD_CACHE,CENTRAL_CACHE,PAGE_HEAP infrastructure
```

## Layer Descriptions

### 1. Application Layer
- **Purpose**: User-facing applications built on NovaLLM runtime
- **Components**:
  - Applications (chatbots, analysis tools, etc.)
  - API Interface (REST, gRPC, etc.)

### 2. Engine Layer
- **Purpose**: Core LLM processing orchestration
- **Components**:
  - **input_processor**: Tokenization, preprocessing
  - **inference**: Model execution and prediction
  - **output_processor**: Result formatting, post-processing

### 3. LLM Inference Layer
- **Purpose**: Neural network model execution
- **Components**:
  - **Model Layer**: Complete neural architecture
    - Network layers (attention, feedforward, etc.)
    - Model weights and parameters

### 4. Base Abstraction Layer
- **Purpose**: Fundamental data structures and abstractions
- **Components**:
  - **Tensor**: Multi-dimensional arrays for ML data
  - **Buffer**: Memory buffer management
  - **Device**: Hardware abstraction (CPU/GPU/NPU)
  - **DataType**: Numerical precision types

### 5. Memory Layer
- **Purpose**: Hardware-specific memory management
- **Components**:

  #### CPU Memory Allocators
  - **StandardAllocator**: Basic malloc/free
  - **TCMallocAllocator**: Google's high-performance allocator
  - **JemallocAllocator**: Facebook's scalable allocator
  - **MimallocAllocator**: Microsoft's modern allocator

  #### GPU Memory Allocators
  - **CUDAAllocator**: NVIDIA CUDA memory management
    - Regular device memory
    - Managed/unified memory

  #### NPU Memory Allocators
  - Specialized allocators for Neural Processing Units

  #### Memory Infrastructure (AMP System)
  - **Arena Router**: Device-specific memory routing
  - **Thread Cache**: Per-thread memory pools
  - **Central Cache**: Shared free lists
  - **Page Heap**: Large allocation handling

## Key Design Principles

1. **Layered Architecture**: Clear separation of concerns
2. **Hardware Abstraction**: Unified interface across CPU/GPU/NPU
3. **Memory Efficiency**: Advanced pooling and caching systems
4. **Extensibility**: Pluggable allocators and modular design
5. **Performance**: High-performance allocators with fallback mechanisms

## Data Flow

```
Application Request
       ↓
    Engine Layer (input → inference → output)
       ↓
  LLM Inference (model execution)
       ↓
Base Abstractions (Tensor, Buffer operations)
       ↓
  Memory Layer (hardware-specific allocation)
       ↓
Hardware Memory (CPU/GPU/NPU physical memory)
