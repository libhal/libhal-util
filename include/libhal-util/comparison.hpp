// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
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

#include <algorithm>
#include <array>
#include <span>

/**
 * @defgroup Comparison Comparisons
 * @{
 */

/**
 * @brief Provides `operator==` between arrays and spans
 *
 * @tparam T - element type of array & span
 * @tparam size - compile time size of the array
 * @param p_array - array object
 * @param p_span - span object
 * @return true - if the two are equal in length and contents
 * @return false - if the two are not equal to length or by contents
 */
template<typename T, size_t size>
constexpr bool operator==(std::array<T, size> const& p_array,
                          std::span<T> const& p_span)
{
  if (p_span.size() != size) {
    return false;
  }

  return std::equal(p_array.begin(), p_array.end(), p_span.begin());
}

/**
 * @brief Provides `operator==` between spans and arrays
 *
 * @tparam T - element type of array & span
 * @tparam size - compile time size of the array
 * @param p_span - span object
 * @param p_array - array object
 * @return true - if the two are equal in length and contents
 * @return false - if the two are not equal to length or by contents
 */
template<typename T, size_t size>
constexpr bool operator==(std::span<T> const& p_span,
                          std::array<T, size> const& p_array)
{
  return p_array == p_span;
}

/**
 * @brief Provides `operator!=` between arrays and spans
 *
 * @tparam T - element type of array & span
 * @tparam size - compile time size of the array
 * @param p_array - array object
 * @param p_span - span object
 * @return true - if the two are equal in length and contents
 * @return false - if the two are not equal to length or by contents
 */
template<typename T, size_t size>
constexpr bool operator!=(std::array<T, size> const& p_array,
                          std::span<T> const& p_span)
{
  return !(p_array == p_span);
}

/**
 * @brief Provides `operator!=` between spans and arrays
 *
 * @tparam T - element type of array & span
 * @tparam size - compile time size of the array
 * @param p_array - array object
 * @param p_span - span object
 * @return true - if the two are equal in length and contents
 * @return false - if the two are not equal to length or by contents
 */
template<typename T, size_t size>
constexpr bool operator!=(std::span<T> const& p_span,
                          std::array<T, size> const& p_array)
{
  return !(p_array == p_span);
}

/** @} */  // End of group
