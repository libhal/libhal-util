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

#include <cstdint>

#include "bit_limits.hpp"

/**
 * @defgroup OverflowCounter Overflow Counter
 *
 */

namespace hal {
/**
 * @ingroup OverflowCounter
 * @brief Extend a counter's count from an arbitrary bit width to 64-bits by
 * detecting overflows in the count. Each detected overflow is added to an
 * overflow counter which is combined with the current count in order create a
 * count up to 64-bits in length.
 *
 * @tparam count_bit_width - the bit width of the counter before the count value
 * overflows.
 */
template<size_t count_bit_width = 32>
class overflow_counter
{
public:
  static_assert(count_bit_width <= 32, "Bit width cannot exceed 32-bits");
  static_assert(count_bit_width > 1, "Bit width must be greater than 1");
  /**
   * @ingroup OverflowCounter
   * @brief update the overflow counter, detect if an overflow has occurred, and
   * return the combined
   *
   * @param p_new_count - must be an increasing value and should only decrease
   * when an overflow event occurs.
   * @return constexpr uint64_t - 64-bit count combining the new count value and
   * the overflow count value.
   */
  constexpr uint64_t update(uint32_t p_new_count)
  {
    // Sanitize the new count value to make sure it does not exceed the
    // designated bit width. Without this check when the count is combined with
    // the overflow value the upper bits may effect the bits of the overflow
    // count, getting an incorrect count value.
    constexpr auto mask = generate_field_of_ones<count_bit_width, uint32_t>();
    p_new_count = p_new_count & mask;

    // Detect the overflow by checking if the new count is smaller than the
    // previous count. If the count is always increasing, the only way for the
    // new count to be smaller is if the count reached the end of its bit width
    // and overflowed.
    if (m_previous_count > p_new_count) {
      m_overflow_count++;
    }

    // Set previous count to the new count
    m_previous_count = p_new_count;

    // Move overflow count up to the upper bits of the 64-bit number based on
    // count_bit_width
    uint64_t combined_count = m_overflow_count;
    combined_count <<= count_bit_width;
    // Add the p_new_count into the combined count to complete it.
    combined_count |= p_new_count;

    return combined_count;
  }

  /**
   * @ingroup OverflowCounter
   * @brief Reset the overflow count back to zero.
   *
   */
  constexpr void reset()
  {
    m_previous_count = 0;
    m_overflow_count = 0;
  }

private:
  uint32_t m_previous_count = 0;
  uint32_t m_overflow_count = 0;
};
}  // namespace hal
