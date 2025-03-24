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
 */

namespace hal {
/**
 * @ingroup AsBytes
 * @brief Converts a pointer and length to a writeable (mutable) span of T
 * elements
 *
 * @tparam T - element type
 * @param p_address - address of the first element
 * @param p_number_of_elements - the number of valid elements after the first
 * @return constexpr std::span<hal::byte> - span of elements
 */
template<typename T>
constexpr std::span<hal::byte> as_writable_bytes(T* p_address,
                                                 size_t p_number_of_elements)
{
  auto* start = reinterpret_cast<hal::byte*>(p_address);
  size_t number_of_bytes = sizeof(T) * p_number_of_elements;
  return { start, number_of_bytes };
}

/**
 * @ingroup AsBytes
 * @brief Converts a pointer and length to a constant span of T elements
 *
 * @tparam T - element type
 * @param p_address - address of the first element
 * @param p_number_of_elements - the number of valid elements after the first
 * @return constexpr std::span<hal::byte const> - span of elements
 */
template<typename T>
constexpr std::span<hal::byte const> as_bytes(T const* p_address,
                                              size_t p_number_of_elements)
{
  auto* start = reinterpret_cast<hal::byte const*>(p_address);
  size_t number_of_bytes = sizeof(T) * p_number_of_elements;
  return { start, number_of_bytes };
}

// Turning off clang-format because concepts because give it an aneurysm.
// clang-format off
/**
 * @brief A concept for determining if something can be used by
 * `as_writable_bytes()` and `as_bytes()`
 *
 * @tparam T - type of the container
 */
template<typename T>
concept convertible_to_bytes = requires(T a) {
                                 *a.data();
                                 a.size();
                               };
// clang-format on

/**
 * @ingroup AsBytes
 * @brief Converts a container to a writable (mutable) span of T elements
 *
 * @tparam T - element type
 * @param p_container - Container containing the contiguous array of elements.
 * @return constexpr std::span<hal::byte const> - span of elements
 */
constexpr std::span<hal::byte> as_writable_bytes(
  convertible_to_bytes auto& p_container)
{
  return as_writable_bytes(p_container.data(), p_container.size());
}

/**
 * @ingroup AsBytes
 * @brief Converts a container to a constant span of T elements
 *
 * @tparam T - element type
 * @param p_container - Container containing the contiguous array of elements.
 * @return constexpr std::span<hal::byte const> - span of elements
 */
constexpr std::span<hal::byte const> as_bytes(
  convertible_to_bytes auto const& p_container)
{
  return as_bytes(p_container.data(), p_container.size());
}
}  // namespace hal
