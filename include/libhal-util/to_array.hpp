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

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

/**
 * @defgroup ToArray To Array
 *
 */
namespace hal {
/**
 * @ingroup ToArray
 * @brief Convert a string_view into a std::array of N number of characters.
 *
 * Will always ensure that the array is null terminated
 *
 * @tparam N - Size of the array
 * @param p_view - string to be placed into a char array
 * @return constexpr std::array<char, N + 1> - the char array object
 */
template<size_t N>
[[nodiscard]] constexpr std::array<char, N + 1> to_array(
  std::string_view p_view)
{
  size_t const min = std::min(N, p_view.size());
  std::array<char, N + 1> result;

  auto out_iterator = std::copy_n(p_view.begin(), min, result.begin());
  std::fill(out_iterator, result.end(), '\0');

  return result;
}
}  // namespace hal
