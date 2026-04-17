// Copyright 2026 Madeline Schneider and the libhal contributors
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

#include <array>
#include <cstddef>
#include <span>
#include <utility>

#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>

namespace hal::v5 {

template<typename T>
constexpr size_t scatter_span_size(scatter_span<T> ss)
{
  size_t res = 0;
  for (auto const& s : ss) {
    res += s.size();
  }

  return res;
}

/**
 * @brief Result of make_sub_scatter_array containing the scatter span array
 * and the number of valid spans within it.
 */
template<typename T, size_t N>
struct sub_scatter_result
{
  // Array of spans composing the sub scatter span
  std::array<std::span<T>, N> spans;
  // Number of valid spans in the array (may be less than N if truncated)
  size_t count;
};

// TODO: Create proper scatter span data structure and remove this
/**
 * @brief Create a sub scatter span from span fragments. Meaning only create a
 * composite scatter span of a desired length instead of being composed of every
 * span.
 *
 * eg:
 * first_span = {1, 2, 3}
 * second_span = {4, 5, 6, 7}
 * new_sub_span = make_scatter_span_array(5, first_span, second_span) => {1, 2,
 * 3, 4, 5}
 *
 * Unfortunately, it is not as clean as the above psuedo code. In reality
 * this function returns the required spans of a given sub scatter span and the
 * number of required spans. As of this time starting location is always the
 * start of the first span given
 *
 * Usage:
 * @code{.cpp}
 * std::array<byte, 3> first = {1, 2, 3};
 * std::array<byte, 4> second = {4, 5, 6, 7};
 * std::array<byte, 2> third = {8, 9};
 *
 * // Request only 5 bytes from 9 total
 * auto result = make_sub_scatter_bytes(5, first, second, third);
 *
 * // result.spans -> Spans in sub scatter span
 * // result.count -> Number of scatter spans needed to account for desired
 * elements
 *
 * // Use count to limit the scatter_span to valid spans only
 * auto ss = scatter_span<byte const>(result.spans).first(result.count);
 * @endcode
 *
 * @tparam T - The type each span contains
 * @tparam Args... - The spans to compose the scatter span
 *
 * @param p_count - Number of elements (of type T) desired for scatter span
 * @param p_spans - The spans that will be used to compose the new sub scatter
 * span.
 *
 * @return A sub_scatter_result containing the spans used within the
 * scatter_span and the number of spans in the sub scatter span.
 */
template<typename T, typename... Args>
constexpr sub_scatter_result<T, sizeof...(Args)> make_sub_scatter_array(
  size_t p_count,
  Args&&... p_spans)
{
  std::array<std::span<T>, sizeof...(Args)> full_ss{ std::span<T>(
    std::forward<Args>(p_spans))... };

  size_t total_span_len = scatter_span_size(scatter_span<T>(full_ss));
  std::array<std::span<T>, sizeof...(Args)> res;
  std::array<size_t, sizeof...(Args)> lens{ std::span<T>(p_spans).size()... };

  if (total_span_len <= p_count) {
    return { .spans = full_ss, .count = full_ss.size() };
  }
  size_t cur_len = 0;
  size_t i = 0;
  for (; i < lens.size(); i++) {
    auto ith_span_length = lens[i];

    if (p_count >= (cur_len + ith_span_length)) {
      res[i] = full_ss[i];
      cur_len += ith_span_length;
      continue;
    }

    if (cur_len >= p_count) {
      return { .spans = res, .count = i };
    }

    auto delta = p_count - cur_len;
    std::span<T> subspan = std::span(full_ss[i]).first(delta);
    res[i] = subspan;
    break;
  }

  return { .spans = res, .count = i + 1 };
}

/**
 * @brief Convenience wrapper for make_sub_scatter_array with byte const type.
 * @see make_sub_scatter_array
 */
template<typename... Args>
constexpr sub_scatter_result<byte const, sizeof...(Args)>
make_sub_scatter_bytes(size_t p_count, Args&&... p_spans)
{
  return make_sub_scatter_array<byte const>(p_count,
                                            std::forward<Args>(p_spans)...);
}

}  // namespace hal::v5
