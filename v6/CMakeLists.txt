# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 4.0)

# TODO(kammce): remove line below
set(LIBHAL_ENABLE_CLANG_TIDY OFF)

project(libhal-util LANGUAGES CXX)

# libhal cmake utilities from libhal-cmake-util
# Source: https://github.com/libhal/libhal-cmake-util
find_package(LibhalCMakeUtil REQUIRED)

find_package(libhal REQUIRED)

libhal_project_init()
libhal_add_library(util
    MODULES
        modules/hal.util.cppm
        modules/as_bytes.cppm
        modules/bit.cppm
        modules/can.cppm
        modules/comparison.cppm
        modules/enum.cppm
        modules/i2c.cppm
        modules/map.cppm
        modules/math.cppm
        modules/overflow_counter.cppm
        modules/serial.cppm
        modules/spi.cppm
        modules/steady_clock.cppm
        modules/to_array.cppm
        modules/units.cppm
    SOURCES
)

target_link_libraries(util
    PUBLIC
        libhal::libhal
)

libhal_apply_compile_options(util)
libhal_install_library(util NAMESPACE libhal)
libhal_add_tests(util
    TEST_NAMES
        main
        as_bytes
        bit
        bit_functions
        can
        enum
        i2c
        map
        math
        overflow_counter
        serial
        spi
        steady_clock
        to_array
        units
    MODULES
        tests/util.cppm
    LINK_LIBRARIES
        libhal::libhal
    PACKAGES
        libhal
)
