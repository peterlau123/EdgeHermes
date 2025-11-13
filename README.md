[![Ubuntu](https://github.com/peterlau123/NovaLLM/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/peterlau123/NovaLLM/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/peterlau123/NovaLLM/actions/workflows/windows.yml/badge.svg)](https://github.com/peterlau123/NovaLLM/actions/workflows/windows.yml)
[![MacOS](https://github.com/peterlau123/NovaLLM/actions/workflows/macos.yml/badge.svg)](https://github.com/peterlau123/NovaLLM/actions/workflows/macos.yml)
[![Code Quality](https://github.com/peterlau123/NovaLLM/actions/workflows/code-quality.yml/badge.svg)](https://github.com/peterlau123/NovaLLM/actions/workflows/code-quality.yml)
[![Documentation](https://github.com/peterlau123/NovaLLM/actions/workflows/documentation.yml/badge.svg)](https://github.com/peterlau123/NovaLLM/actions/workflows/documentation.yml)
[![codecov](https://codecov.io/gh/peterlau123/NovaLLM/branch/master/graph/badge.svg)](https://codecov.io/gh/peterlau123/NovaLLM)

<p align="center">
  <img src="documentation/images/NovaLLM_logo.png" height="200" width="250" />
</p>

# NovaLLM

A lightweight and efficient C/C++ library for Large Language Model (LLM) inference. The name **Nova** reflects our goal to bring a new, powerful, and efficient approach to LLM deployment, making it accessible everywhere.

## Features

- 🚀 **Lightweight**: Minimal dependencies, focusing on core functionality
- 🔧 **Extensible**: Easy to extend with custom models and optimizations
- 🎯 **Efficient**: Support for extreme low-bit quantization
- 🛠️ **Flexible**: Support for OpenAI Triton and ThunderKittens kernels

## Supported Models

### Language Models

| Model | Parameters | Status |
|-------|------------|--------|
| Qwen | 1.8B | 🟡 In Development |
| | 7B | ⚪ Planned |
| | 14B | ⚪ Planned |
| DeepSeek | 7B | ⚪ Planned |
| | 67B | ⚪ Planned |
| Llama | 7B | ⚪ Planned |

### Vision Models
*Coming soon...*

## Quick Start

### Prerequisites

- CMake 3.14 or higher
- C++17 compatible compiler
- Conan package manager
- Python 3.10+ (for Conan)

### Building

1. **Clone the repository**
```bash
git clone https://github.com/peterlau123/NovaLLM.git
cd NovaLLM
```

2. **Install dependencies and build**
```bash
# Create and enter build directory
mkdir build && cd build

# Install dependencies with Conan
conan install .. --output-folder=. --build=missing

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Scripted builds

Unified wrapper (auto-detects OS):

```bash
scripts/build.sh --type Release --enable-logging ON --with-tests
```

Or call platform-specific scripts:

```bash
# macOS
scripts/build_macos.sh --type Release --enable-logging ON --with-tests
# Ubuntu/Linux
scripts/build_ubuntu.sh --type Debug --enable-logging OFF
# Windows (PowerShell)
scripts/build_windows.ps1 -Configuration Release -EnableLogging ON -WithTests
```

### Makefile builds
- Use scripts via Make: `make script-build` (honors BUILD_TYPE, ENABLE_LOGGING, ENABLE_TESTS)

```bash
# Build and install (Release by default)
make install
# Debug build with logging disabled
make BUILD_TYPE=Debug ENABLE_LOGGING=OFF install
# Build & run tests (scripted)
make ENABLE_TESTS=ON script-test
```

```bash
# Build and install (Release by default)
make install

# Debug build with logging disabled
make BUILD_TYPE=Debug ENABLE_LOGGING=OFF install

# Build & run tests
make ENABLE_TESTS=ON test
```

3. **Run tests**
```bash
# Run all tests
ctest --output-on-failure

# Run specific test
./bin/NovaLLM_tests
```

### Development

#### Code Style

We use clang-format for code formatting. To format your code:

```bash
# Install clang-format
pip install clang-format==14.0.6

# Format code
cmake --build build --target fix-format
```

#### Building Documentation

```bash
# Build documentation
cmake --build build --target docs

# View documentation
open build/docs/html/index.html
```

## Advanced Usage

### Model Quantization

```cpp
#include <NovaLLM/quantization.hpp>

// Example quantization code
auto quantized_model = NovaLLM::quantize_model(model, NovaLLM::QuantizationType::INT8);
```

### Custom Kernel Integration

```cpp
#include <NovaLLM/kernels.hpp>

// Example custom kernel usage
NovaLLM::register_custom_kernel("my_kernel", kernel_function);
```

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [OpenAI Triton](https://github.com/openai/triton) for kernel optimizations
- [ThunderKittens](https://github.com/HazyResearch/ThunderKittens) for kernel implementations

## Contact

- GitHub Issues: [Create an issue](https://github.com/peterlau123/NovaLLM/issues)
- Email: [Your email]

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=peterlau123/NovaLLM&type=Date)](https://star-history.com/#peterlau123/NovaLLM&Date)
