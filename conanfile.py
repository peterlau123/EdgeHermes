import os
from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout, CMakeDeps, CMakeToolchain
from conan.tools.files import copy

class NovallmConan(ConanFile):
    name = "novallm"
    version = "0.1.0" # Match your project version
    exports_sources = "CMakeLists.txt", "source/*", "include/*", "cmake/*"
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_logging": [True, False], # Corresponds to NOVA_LLM_ENABLE_LOGGING
        "build_tests": [True, False], # Corresponds to NOVA_LLM_BUILD_TESTS
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_logging": True,
        "build_tests": False,
    }

    # Requirements - these are the dependencies your project uses
    def requirements(self):
        self.requires("fmt/10.2.1")
        if self.options.enable_logging:
            self.requires("spdlog/1.12.0")
        if self.options.build_tests:
            self.requires("gtest/1.12.1")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["NOVA_LLM_ENABLE_LOGGING"] = self.options.enable_logging
        tc.variables["NOVA_LLM_BUILD_TESTS"] = self.options.build_tests
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "NovaLLM")
        self.cpp_info.set_property("cmake_target_name", "NovaLLM::NovaLLM")
        self.cpp_info.libs = ["NovaLLM"]

    # Note: For a project conanfile.py, you typically don't implement build(), package(), etc.
    # Those are for creating packages of YOUR project. This conanfile is just for managing requirements. 