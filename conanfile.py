#!/usr/bin/python
#
# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.files import copy
from conan.tools.build import check_min_cppstd
import os


required_conan_version = ">=2.0.14"


class libhal_util_conan(ConanFile):
    name = "libhal-util"
    license = "Apache-2.0"
    homepage = "https://github.com/libhal/libhal-util"
    description = ("A collection of interfaces and abstractions for embedded "
                   "peripherals and devices using modern C++")
    topics = ("peripherals", "hardware", "abstraction", "devices", "hal")
    settings = "compiler", "build_type", "os", "arch"
    exports_sources = ("include/*", "tests/*", "LICENSE",
                       "CMakeLists.txt", "src/*")
    generators = "CMakeToolchain", "CMakeDeps"

    @property
    def _min_cppstd(self):
        return "20"

    @property
    def _compilers_minimum_version(self):
        return {
            "gcc": "11",
            "clang": "14",
            "apple-clang": "14.0.0"
        }

    @property
    def _bare_metal(self):
        return self.settings.os == "baremetal"

    def validate(self):
        if self.settings.get_safe("compiler.cppstd"):
            check_min_cppstd(self, self._min_cppstd)

    def build_requirements(self):
        self.tool_requires("cmake/3.27.1")
        self.tool_requires("libhal-cmake-util/[^4.1.0]")
        self.test_requires("boost-ext-ut/2.1.0")

    def requirements(self):
        self.requires("libhal/[^4.9.0]", transitive_headers=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build(target="copy_compile_commands")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self,
             "LICENSE",
             dst=os.path.join(self.package_folder, "licenses"),
             src=self.source_folder)
        copy(self,
             "*.h",
             dst=os.path.join(self.package_folder, "include"),
             src=os.path.join(self.source_folder, "include"))
        copy(self,
             "*.hpp",
             dst=os.path.join(self.package_folder, "include"),
             src=os.path.join(self.source_folder, "include"))

        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["libhal-util"]
        self.cpp_info.set_property("cmake_target_name", "libhal::util")
