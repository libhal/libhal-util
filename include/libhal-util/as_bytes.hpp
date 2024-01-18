// Copyright 2024 Khalil Estell
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <span>

#include <libhal/units.hpp>

/**
 * @defgroup AsBytes As Bytes
 @verbatim embed:rst
 ```{warning}
  Documentation is needed for this file, please add each public facing API to
 this group!
 ```
 @endverbatim
 *
 *
 */

namespace hal {
template<typename T>
constexpr std::span<hal::byte> as_writable_bytes(T* p_address,
                                                 size_t p_number_of_elements)
{
  auto* start = reinterpret_cast<hal::byte*>(p_address);
  size_t number_of_bytes = sizeof(T) * p_number_of_elements;
  return std::span<hal::byte>(start, number_of_bytes);
}

template<typename T>
constexpr std::span<const hal::byte> as_bytes(const T* p_address,
                                              size_t p_number_of_elements)
{
  auto* start = reinterpret_cast<const hal::byte*>(p_address);
  size_t number_of_bytes = sizeof(T) * p_number_of_elements;
  return std::span<const hal::byte>(start, number_of_bytes);
}

template<typename T>
concept convertible_to_bytes = requires(T a) {
                                 *a.data();
                                 a.size();
                               };

constexpr std::span<hal::byte> as_writable_bytes(
  convertible_to_bytes auto& p_container)
{
  return as_writable_bytes(p_container.data(), p_container.size());
}

constexpr std::span<const hal::byte> as_bytes(
  const convertible_to_bytes auto& p_container)
{
  return as_bytes(p_container.data(), p_container.size());
}
}  // namespace hal
