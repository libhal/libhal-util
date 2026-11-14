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

#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>

/**
 * @defgroup BitLimits Bit Limits
 *
 */

namespace hal {
/**
 * @ingroup BitLimits
 * @brief Generate a mask of 1s at compile time
 *
 * @tparam BitWidth - number of 1s in the mask
 * @tparam T - the type
 * @return consteval uint32_t - mask with 1s at the LSB
 */
template<size_t BitWidth, std::integral T>
[[nodiscard]] consteval T generate_field_of_ones()
{
  T result = 0;
  for (size_t i = 0; i < BitWidth; i++) {
    result |= 1 << i;
  }
  return result;
}

/**
 * @ingroup BitLimits
 * @brief Similar to std::numeric_limits<T> except that this object can give
 * properties of integral types of arbitrary bit widths.
 *
 * @tparam BitWidth - bit width of the value
 * @tparam int_t - the containing type. Will generate a compile error if the
 * containing type's bit width is smaller than the BitWidth value.
 */
template<size_t BitWidth, std::integral int_t>
struct bit_limits
{
  // Check that the bit width is less than or equal to the size of the int_t.
  static_assert(BitWidth <= sizeof(int_t) * 8,
                "The BitWidth exceed the number of bits in the integer type.");

  // Check that bit width is not zero.
  static_assert(BitWidth != 0, "The BitWidth cannot be 0.");

  /**
   * @ingroup BitLimits
   * @brief Get the maximum value available for an integer of `BitWidth` size
   * and that can be stored within `int_t`.
   *
   * The final value of the function depends also on the sign of the int type.
   *
   * @return constexpr int_t maximum value
   */
  [[nodiscard]] static constexpr int_t max()
  {
    using unsigned_t = std::make_unsigned_t<int_t>;
    if constexpr (BitWidth == sizeof(int_t) * 8) {
      return std::numeric_limits<int_t>::max();
    } else if constexpr (std::is_signed_v<int_t>) {
      // A signed field spends one of its bits on the sign.
      return static_cast<int_t>((unsigned_t{ 1 } << (BitWidth - 1)) - 1U);
    } else {
      return static_cast<int_t>((unsigned_t{ 1 } << BitWidth) - 1U);
    }
  }

  /**
   * @ingroup BitLimits
   * @brief Get the minimum value available for an integer of `BitWidth` size
   * and that can be stored within `int_t`.
   *
   * The final value of the function depends also on the sign of the int type.
   * Unsigned ints simply return zero.
   *
   * @return constexpr int_t minimum value
   */
  [[nodiscard]] static constexpr int_t min()
  {
    if constexpr (std::is_signed_v<int_t>) {
      // Two's complement reaches one below the negated maximum.
      return static_cast<int_t>(-max() - 1);
    } else {
      return 0;
    }
  }
};
}  // namespace hal
