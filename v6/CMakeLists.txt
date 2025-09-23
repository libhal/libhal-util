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

cmake_minimum_required(VERSION 3.28)

project(hal-util LANGUAGES CXX)

# Require Ninja or Visual Studio for modules
if(NOT CMAKE_GENERATOR MATCHES "Ninja|Visual Studio")
    message(FATAL_ERROR "C++20 modules require Ninja or Visual Studio generator")
endif()

# Create the library target
add_library(hal-util)

find_package(libhal REQUIRED)

# Add module files
target_sources(hal-util PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES
    modules/hal.util.cppm
    modules/math.cppm
    modules/to_array.cppm
)

target_compile_features(hal-util PUBLIC cxx_std_23)
target_link_libraries(hal-util PUBLIC libhal::libhal)

install(
    TARGETS hal-util
    EXPORT hal-util_targets
    FILE_SET CXX_MODULES DESTINATION .
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/hal-util.dir/
    DESTINATION modules
    FILES_MATCHING PATTERN "*.pcm"
)

# ==============================================================================
# Unit testing
# ==============================================================================

find_package(ut REQUIRED)

add_executable(unit_test)

# Add module files
target_sources(unit_test
  PRIVATE
      tests/main.test.cpp
)

target_compile_features(unit_test PUBLIC cxx_std_23)
target_link_libraries(unit_test PRIVATE hal-util boost-ext-ut::ut)

# Run unit tests
add_custom_target(run_tests ALL DEPENDS unit_test COMMAND unit_test)
