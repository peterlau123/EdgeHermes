# NovaLLM Architecture Overview

## Architecture Diagram (积木式分层结构)

```mermaid
flowchart TD
    %% Application Layer - Top Block
    subgraph APP_BLOCK["📱 Application Layer<br/>应用层"]
        A1[Applications<br/>应用]
        A2[API Interface<br/>API接口]
    end

    %% Engine Layer - Second Block
    subgraph ENGINE_BLOCK["⚙️ Engine Layer<br/>引擎层"]
        E1[input_processor<br/>输入处理器]
        E2[inference<br/>推理引擎]
        E3[output_processor<br/>输出处理器]
    end

    %% LLM Inference Layer - Third Block
    subgraph INFERENCE_BLOCK["🧠 LLM Inference Layer<br/>LLM推理层"]
        I1[Model<br/>模型架构]
        I2[Layers<br/>网络层]
        I3[Weights<br/>权重参数]
    end

    %% Base Abstraction Layer - Fourth Block
    subgraph ABSTRACTION_BLOCK["🏗️ Base Abstraction Layer<br/>基础抽象层"]
        B1[Tensor<br/>张量]
        B2[Buffer<br/>缓冲区]
        B3[Device<br/>设备]
        B4[DataType<br/>数据类型]
    end

    %% Memory Layer - Bottom Block
    subgraph MEMORY_BLOCK["💾 Memory Layer<br/>内存层"]
        subgraph CPU_MEM["🖥️ CPU Memory<br/>CPU内存"]
            C1[StandardAllocator]
            C2[TCMallocAllocator]
            C3[JemallocAllocator]
            C4[MimallocAllocator]
        end

        subgraph GPU_MEM["🎮 GPU Memory<br/>GPU内存"]
            G1[CUDAAllocator]
            G2[Managed Memory]
            G3[Device Memory]
        end

        subgraph NPU_MEM["🔧 NPU Memory<br/>NPU内存"]
            N1[NPU Allocators]
        end

        subgraph INFRA["🏛️ Memory Infrastructure<br/>内存基础设施"]
            M1[AMP System]
            M2[Arena Router]
            M3[Thread Cache]
            M4[Central Cache]
            M5[Page Heap]
        end
    end

    %% Layer Connections (积木堆叠)
    APP_BLOCK --> ENGINE_BLOCK
    ENGINE_BLOCK --> INFERENCE_BLOCK
    INFERENCE_BLOCK --> ABSTRACTION_BLOCK
    ABSTRACTION_BLOCK --> MEMORY_BLOCK

    %% Internal Connections
    E1 --> E2 --> E3
    I1 --> I2 --> I3
    B1 --> B2 --> B3 --> B4

    C1 --> C2 --> C3 --> C4
    G1 --> G2 --> G3
    M1 --> M2 --> M3 --> M4 --> M5

    %% Data Flow Arrows
    A1 -.->|API调用| A2
    A2 -.->|请求处理| E1
    E2 -.->|模型推理| I1
    I2 -.->|张量运算| B1
    B2 -.->|内存分配| C1
    B2 -.->|GPU内存| G1

    %% Styling - 积木风格
    classDef appBlock fill:#e3f2fd,stroke:#1976d2,stroke-width:3px,color:#000
    classDef engineBlock fill:#f3e5f5,stroke:#7b1fa2,stroke-width:3px,color:#000
    classDef inferenceBlock fill:#e8f5e8,stroke:#388e3c,stroke-width:3px,color:#000
    classDef abstractionBlock fill:#fff3e0,stroke:#f57c00,stroke-width:3px,color:#000
    classDef memoryBlock fill:#fce4ec,stroke:#c2185b,stroke-width:3px,color:#000
    classDef component fill:#ffffff,stroke:#666,stroke-width:1px,color:#000

    class APP_BLOCK appBlock
    class ENGINE_BLOCK engineBlock
    class INFERENCE_BLOCK inferenceBlock
    class ABSTRACTION_BLOCK abstractionBlock
    class MEMORY_BLOCK memoryBlock
    class A1,A2,E1,E2,E3,I1,I2,I3,B1,B2,B3,B4,C1,C2,C3,C4,G1,G2,G3,N1,M1,M2,M3,M4,M5 component
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
