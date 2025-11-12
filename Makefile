# NovaLLM Makefile
# Usage: make [target] [options]
# Options:
#   BUILD_TYPE=Debug|Release (default: Release)
#   ENABLE_TESTS=ON|OFF (default: OFF)   # influences dependencies via Conan
#   ENABLE_LOGGING=ON|OFF (default: ON)  # passes -DNOVA_LLM_ENABLE_LOGGING to CMake
#   INSTALL_DIR=<path>                   # default: install or install-<type>
#   CLEAN=1                              # clean build directory before building

# Defaults
BUILD_TYPE ?= Release
ENABLE_TESTS ?= OFF
ENABLE_LOGGING ?= ON
CLEAN ?= 0

# Derived dirs (suffix by build type)
SUFFIX := $(shell echo $(BUILD_TYPE) | tr A-Z a-z)
BUILD_DIR ?= build-$(SUFFIX)
INSTALL_DIR ?= install-$(SUFFIX)

# Detect number of CPU cores for parallel builds
NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

# Parallel build support
ifneq (,$(filter -j%,$(MAKEFLAGS)))
    JOBS := $(patsubst -j%,%,$(filter -j%,$(MAKEFLAGS)))
else
    JOBS := $(NPROC)
endif
BUILD_FLAGS := -j$(JOBS)

# ANSI color codes
GREEN := \033[32m
YELLOW := \033[33m
RED := \033[31m
RESET := \033[0m

# Print functions
print_info = @echo "$(GREEN)$(1)$(RESET)"
print_warn = @echo "$(YELLOW)$(1)$(RESET)"
print_error = @echo "$(RED)$(1)$(RESET)"

# Check if required tools are installed
check_tools:
	@command -v cmake >/dev/null 2>&1 || { $(call print_error,"Error: cmake is required but not installed."); exit 1; }
	@command -v conan >/dev/null 2>&1 || { $(call print_error,"Error: conan is required but not installed."); exit 1; }

# Create build directory
$(BUILD_DIR):
	$(call print_info,"Creating build directory '$(BUILD_DIR)' ...")
	@mkdir -p $(BUILD_DIR) $(INSTALL_DIR)

# Clean build & test directories
clean:
	$(call print_info,"Cleaning build directories...")
	@rm -rf build* install* cppcheck-report.txt clang-tidy-report.txt

# Configure project (Conan deps + CMake configure with toolchain)
configure: check_tools $(BUILD_DIR)
	$(call print_info,"Configuring (BUILD_TYPE=$(BUILD_TYPE), TESTS=$(ENABLE_TESTS), LOGGING=$(ENABLE_LOGGING)) ...")
	@conan profile detect --force >/dev/null 2>&1 || true
	@cd $(BUILD_DIR) && \
	  conan install .. --output-folder=. --build=missing -s build_type=$(BUILD_TYPE) -o build_tests=$$( [ "$(ENABLE_TESTS)" = "ON" ] && echo True || echo False )
	@TOOLCHAIN_FILE=$$(find $(BUILD_DIR) -name conan_toolchain.cmake -type f | head -1) && \
	  cmake -S . -B $(BUILD_DIR) >/dev/null 2>&1 || true # noop to avoid warnings
	@TOOLCHAIN_FILE=$$(find $(BUILD_DIR) -name conan_toolchain.cmake -type f | head -1) && \
	  cmake -S . -B . >/dev/null 2>&1 || true
	@TOOLCHAIN_FILE=$$(find $(BUILD_DIR) -name conan_toolchain.cmake -type f | head -1); \
	  if [ -z "$$TOOLCHAIN_FILE" ]; then $(call print_error,"conan_toolchain.cmake not found in $(BUILD_DIR)"); exit 1; fi; \
	  cmake -S . -B $(BUILD_DIR) >/dev/null 2>&1 || true
	@cd $(BUILD_DIR) && \
	  cmake -S .. -B . \
	    -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	    -DNOVA_LLM_ENABLE_LOGGING=$(ENABLE_LOGGING) \
	    -DCMAKE_INSTALL_PREFIX=$$(cd .. && pwd)/$(INSTALL_DIR) \
	    -DCMAKE_TOOLCHAIN_FILE=$$(find . -name conan_toolchain.cmake -type f | head -1)

# Build project
build: configure
	$(call print_info,"Building with $(JOBS) jobs ...")
	@cmake --build $(BUILD_DIR) --config $(BUILD_TYPE) $(BUILD_FLAGS)

# Install project
install: build
	$(call print_info,"Installing to '$(INSTALL_DIR)' ...")
	@cmake --install $(BUILD_DIR) --config $(BUILD_TYPE)

# Build & run tests (separate CMake project under test/)
TEST_BUILD_DIR := build-test-$(SUFFIX)

test: check_tools install
	$(call print_info,"Configuring/Building tests ...")
	@mkdir -p $(TEST_BUILD_DIR)
	@cd $(TEST_BUILD_DIR) && conan install ../test --output-folder=conan --build=missing -s build_type=$(BUILD_TYPE)
	@cd $(TEST_BUILD_DIR) && cmake -S ../test -B . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_TOOLCHAIN_FILE=$$(find . -name conan_toolchain.cmake -type f | head -1)
	@cmake --build $(TEST_BUILD_DIR) --config $(BUILD_TYPE) $(BUILD_FLAGS)
	@([ -f $(TEST_BUILD_DIR)/CTestTestfile.cmake ] && cd $(TEST_BUILD_DIR) && ctest --output-on-failure || true)

# Generate documentation
docs: build
	$(call print_info,"Generating documentation ...")
	@cmake --build $(BUILD_DIR) --target docs --config $(BUILD_TYPE)

# Create a local Conan package of this project
package: install
	$(call print_info,"Creating Conan package (build_type=$(BUILD_TYPE)) ...")
	@conan create . --user=local --channel=testing --build=missing -s build_type=$(BUILD_TYPE)

# Scripted build wrappers (call scripts/build.sh)
SCRIPT_ARGS := --type $(BUILD_TYPE) --enable-logging $(ENABLE_LOGGING) --install-prefix $(INSTALL_DIR)
ifeq ($(ENABLE_TESTS),ON)
SCRIPT_ARGS += --with-tests
endif

script-build:
	$(call print_info,"Running scripts/build.sh $(SCRIPT_ARGS) ...")
	@bash scripts/build.sh $(SCRIPT_ARGS)

script-test:
	@$(MAKE) script-build ENABLE_TESTS=ON

# Show build configuration
config:
	$(call print_info,"Build Configuration:")
	@echo "  Build Type:   $(BUILD_TYPE)"
	@echo "  Build Tests:  $(ENABLE_TESTS)"
	@echo "  Logging:      $(ENABLE_LOGGING)"
	@echo "  Build Dir:    $(BUILD_DIR)"
	@echo "  Install Dir:  $(INSTALL_DIR)"
	@echo "  Jobs:         $(JOBS)"

# Show help
help:
	$(call print_info,"NovaLLM Makefile Usage:")
	@echo "make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all           Build the project (default)"
	@echo "  build         Configure and build the project"
	@echo "  clean         Remove build directories and reports"
	@echo "  install       Build and install the project"
	@echo "  test          Build and (optionally) run tests"
	@echo "  docs          Generate documentation"
	@echo "  package       Create a Conan package of NovaLLM"
	@echo "  script-build  Run scripts/build.sh with current Make variables"
	@echo "  script-test   Run scripts/build.sh with tests enabled"
	@echo "  config        Show current build configuration"
	@echo "  help          Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=Debug|Release    Build type (default: Release)"
	@echo "  ENABLE_TESTS=ON|OFF        Pull test deps via Conan (default: OFF)"
	@echo "  ENABLE_LOGGING=ON|OFF      Toggle logging (default: ON)"
	@echo "  INSTALL_DIR=path           Custom install dir (default: install-<type>)"
	@echo "  CLEAN=1                    Clean build dir before building"
	@echo ""
	@echo "Examples:"
	@echo "  make BUILD_TYPE=Debug"
	@echo "  make ENABLE_LOGGING=OFF"
	@echo "  make test BUILD_TYPE=Debug"
	@echo "  make script-build BUILD_TYPE=Release ENABLE_TESTS=ON"

# Default target
all: 
	@if [ "$(CLEAN)" = "1" ]; then $(MAKE) clean; fi
	@$(MAKE) build

.PHONY: all build clean install docs config help check_tools configure test package script-build script-test
