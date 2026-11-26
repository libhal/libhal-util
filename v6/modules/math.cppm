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

/**
 * @file math.hpp
 * @brief Compile time generic math utility functions
 * @defgroup Math Math
 */
export module hal.util:math;

import std;

import hal;

namespace hal::inline v6 {
/**
 * @ingroup Math
 * @brief Perform multiply operation and return an error code
 * `std::errc::result_out_of_range` if the two values when multiplied would
 * overflow the containing value.
 *
 * @tparam T - integer arithmetic type
 * @param p_lhs - left hand side integer
 * @param p_rhs - right hand side integer
 * @returns std::optional<T> - the resultant, std::nullopt if the operation
 * overflows.
 */
export template<typename T>
[[nodiscard]] std::optional<T> multiply(T p_lhs, T p_rhs)
{
  if (p_lhs == 0 || p_rhs == 0) {
    return T{ 0 };
  }

  T const result = p_lhs * p_rhs;

  if (p_lhs != result / p_rhs) {
    return std::nullopt;
  }

  return result;
}

/**
 * @ingroup Math
 * @brief Generic absolute value function that works for integer types.
 *
 * Preferred this over the C API for rounding numbers such as abs(), labs() and
 * llabs(). This function relieves the need in template code to check the type
 * of the integer and select the correct function to call.
 *
 * NOTE: If p_value is minimum negative number of type T then the resulting
 * return value will be the maximum positive number represented by T. For
 * example, INT32_MIN is 2147483648 where as INT32_MAX is 2147483647. The
 * absolute value of INT32_MIN is 1 greater than INT32_MAX. To prevent overflow,
 * passing INT32_MIN will simply return back INT32_MAX.
 *
 * @tparam T - integral type
 * @param p_value - integer value to be made positive
 * @return T - positive representation of the integer
 */
export template<typename T>
[[nodiscard]] constexpr T absolute_value(T p_value) noexcept
{
  if constexpr (std::is_unsigned_v<T>) {
    return p_value;
  } else {
    if (p_value >= 0) {
      return p_value;
    }
    if (p_value == std::numeric_limits<T>::min()) {
      return std::numeric_limits<T>::max();
    }
    return -p_value;
  }
}

/**
 * @ingroup Math
 * @brief How `hal::divide()` rounds a quotient that isn't a whole number.
 *
 * The modes round the way `std::chrono::floor`, `std::chrono::ceil` and
 * `std::chrono::round` round a duration. Truncation, rounding toward zero, is
 * plain `/`, as `std::chrono::duration_cast` truncates, so it isn't a mode. For
 * unsigned types, `floor` is the same as `/`.
 */
export enum class rounding : u8 {
  /// Toward negative infinity: 7 / 2 is 3, and -7 / 2 is -4
  floor,
  /// Toward positive infinity: 7 / 2 is 4, and -7 / 2 is -3
  ceil,
  /// To the nearest integer, a half to the even neighbor: 7 / 3 is 2, 5 / 2
  /// is 2, 7 / 2 is 4, and -5 / 2 is -2
  round,
};

/**
 * @ingroup Math
 * @brief The two operands of a division, named so a call site can't swap them.
 *
 * @tparam T - integral type of the operands
 */
export template<std::integral T>
struct division
{
  /// The value to divide (the numerator)
  T num;
  /// The value to divide by (the denominator)
  T den;
};

/**
 * @ingroup Math
 * @brief Integer division that rounds the quotient as `Mode` says.
 *
 * Use `rounding::ceil` where a result must never fall short: a clock divider
 * that keeps a bus from running faster than asked, or a tick count for a wait
 * that must not end early. Unlike `(num + den - 1) / den`, it can't overflow.
 *
 * Give the result type and name the operands:
 *
 *     auto const counts = hal::divide<hal::rounding::ceil, hal::u64>(
 *       { .num = cycles, .den = max_count });
 *
 * Inside the braces, an operand that would narrow into `T` is a compile error.
 *
 * A denominator of 0 divides by 1 instead, so the result is the numerator. In a
 * constant expression, a denominator of 0 is a compile error.
 *
 * For a signed `T`, the smallest value divided by -1 doesn't fit in `T`, the
 * same as for `/`.
 *
 * @tparam Mode - how to round a quotient that isn't a whole number
 * @tparam T - integral type of the operands and the result
 * @param p_terms - the numerator and denominator
 * @return T - the rounded quotient
 */
export template<rounding Mode, std::integral T>
[[nodiscard]] constexpr T divide(division<T> p_terms)
{
  using unsigned_t = std::make_unsigned_t<T>;

  if (p_terms.den == 0) {
    if consteval {
      // Divides by zero, so the compiler reports a division by zero.
      [[maybe_unused]] auto const zero_denominator = p_terms.num / p_terms.den;
    }
    p_terms.den = 1;
  }

  // For an unsigned type, floor and ceil need only the quotient. Finding the
  // remainder too would cost extra multiplies where the division is a library
  // call, such as 64-bit division on a 32-bit core.
  if constexpr (std::is_unsigned_v<T> and (Mode != rounding::round)) {
    if constexpr (Mode == rounding::floor) {
      return static_cast<T>(p_terms.num / p_terms.den);
    } else {
      // ceil(n / d) is ((n - 1) / d) + 1 for n > 0, and can't overflow.
      if (p_terms.num == 0) {
        return 0;
      }
      return static_cast<T>(((p_terms.num - 1U) / p_terms.den) + 1U);
    }
  }

  // Work with the magnitudes in the unsigned type, which holds the magnitude
  // of the smallest signed value, and apply the sign at the end.
  auto const magnitude = [](T p_value) -> unsigned_t {
    auto const raw = static_cast<unsigned_t>(p_value);
    if constexpr (std::is_signed_v<T>) {
      if (p_value < 0) {
        return static_cast<unsigned_t>(unsigned_t{ 0 } - raw);
      }
    }
    return raw;
  };

  bool negative = false;
  if constexpr (std::is_signed_v<T>) {
    negative = (p_terms.num < 0) != (p_terms.den < 0);
  }

  auto const numerator = magnitude(p_terms.num);
  auto const denominator = magnitude(p_terms.den);
  unsigned_t quotient = numerator / denominator;
  unsigned_t const remainder = numerator % denominator;

  // The magnitude is truncated, so each mode decides whether to step it away
  // from zero.
  bool step = false;
  if constexpr (Mode == rounding::floor) {
    step = negative and (remainder != 0);
  } else if constexpr (Mode == rounding::ceil) {
    step = (not negative) and (remainder != 0);
  } else {
    // Compares the remainder with `denominator - remainder`, which can't
    // overflow like `remainder * 2`. More than a half steps; exactly a half
    // steps only to make the quotient even.
    auto const rest = denominator - remainder;
    bool const odd = (quotient & 1U) != 0;
    step = (remainder > rest) or ((remainder == rest) and odd);
  }
  if (step) {
    quotient++;
  }

  return negative ? static_cast<T>(unsigned_t{ 0 } - quotient)
                  : static_cast<T>(quotient);
}

/**
 * @ingroup Math
 * @brief Calculates the distance between two values (L1 Norm or Manhattan
 * distance), the absolute value of their difference.
 *
 * @tparam T - integral type of the two values
 * @param p_left - the first point of the distance calculation
 * @param p_right - the second point of the distance calculation
 * @returns constexpr T - absolute value of the difference between the two
 * points.
 */
export template<typename T>
[[nodiscard]] constexpr T distance(T p_left, T p_right)
{
  if (p_right > p_left) {
    return p_right - p_left;
  } else {
    return p_left - p_right;
  }
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters):
//
//    Why this NOTLINT?
//
//    I allow this for this API because both sides of the function can be
//    safely swapped and the correct answer is provided.

/**
 * @ingroup Math
 * @brief Calculates the distance between two values (L1 Norm or Manhattan
 * distance), the absolute value of their difference.
 *
 * NOTE: Values cannot exceed int32_t.
 *
 * @tparam T - integral type of the two values
 * @param p_left - the first point of the distance calculation
 * @param p_right - the second point of the distance calculation
 * @returns constexpr T - absolute value of the difference between the two
 * points.
 */
export template<std::integral T>
[[nodiscard]] constexpr std::make_unsigned_t<T> distance(T p_left, T p_right)
{
  // NOLINTEND(bugprone-easily-swappable-parameters)
  static_assert(sizeof(T) <= sizeof(std::int32_t));

  using unsigned_t = std::make_unsigned_t<T>;
  // Put left and right values into 64-bit containers to prevent overflow
  std::int64_t const left{ p_left };
  std::int64_t const right{ p_right };

  if (right > left) {
    // Subtraction operation on right to left in this order can never overflow
    // because the maximum resultant of left and right being INT32_MAX and
    // INT32_MIN, will equal UINT32_MAX which can be stored within an int64_t
    // value.
    std::int64_t const difference = right - left;
    // Casting this value to the unsigned variant will always fit as the
    // distance between any 32-bit signed numbers can always fit in a 32-bit
    // unsigned number.
    return static_cast<unsigned_t>(difference);
  } else {
    // Same logic as the if statement block above.
    std::int64_t const difference = left - right;
    return static_cast<unsigned_t>(difference);
  }
}

/**
 * @ingroup Math
 * @brief Determines if two values are equal within a relative error.
 *
 * @param p_value1 - First value to compare.
 * @param p_value2 - Second value to compare.
 * @param p_epsilon - Error margin that the difference is compared to.
 * @return true - difference is less than epsilon
 * @return false - difference is more than epsilon
 */
export [[nodiscard]] constexpr bool equals(std::floating_point auto p_value1,
                                           std::floating_point auto p_value2,
                                           float p_epsilon = 1e-9f)
{
  if (p_value1 == p_value2) {
    return true;
  }
  auto const value1_abs = std::abs(p_value1);
  auto const value2_abs = std::abs(p_value2);
  auto const diff = std::abs(p_value1 - p_value2);
  auto const absolute_values_sum = value1_abs + value2_abs;

  using float_t = decltype(absolute_values_sum);

  if (p_value1 == 0 || p_value2 == 0 ||
      (absolute_values_sum < std::numeric_limits<float_t>::min())) {
    return diff < (p_epsilon * std::numeric_limits<float_t>::min());
  } else {
    auto const relative_error =
      diff / std::min(absolute_values_sum, std::numeric_limits<float_t>::max());
    return relative_error < p_epsilon;
  }
}

/**
 * @ingroup Math
 * @brief Proportionally scale input value up to fill the desired output integer
 * container.
 *
 * @tparam int_t - output integer storage to be filled with the proportional
 * contents of p_value
 * @tparam bit_width - width of the input value
 * @param p_value - the number to scale to the size of the int_t container
 * @return constexpr int_t -
 */
export template<std::unsigned_integral int_t, usize bit_width>
[[nodiscard]] constexpr int_t upscale(int_t p_value)
{
  constexpr usize resultant_bit_width = std::numeric_limits<int_t>::digits;
  static_assert(bit_width > 0 && bit_width <= resultant_bit_width,
                "Bit width must be between 1 and 32");
  // If already 32 bits, return as-is
  if constexpr (bit_width == resultant_bit_width) {
    return p_value;
  }

  // Create mask for the input bits
  constexpr auto over_sized_mask = (1ULL << bit_width) - 1ULL;
  constexpr auto mask = static_cast<int_t>(over_sized_mask);

  // Calculate number of iterations needed (ceiling(32/bit_width) - 1)
  constexpr auto iterations =
    (resultant_bit_width + bit_width - 1) / bit_width - 1;

  // Place cleaned input value in MSB position
  constexpr auto shift_distance = resultant_bit_width - bit_width;
  int_t result = (p_value & mask) << shift_distance;

  // Replicate the pattern for the calculated number of iterations
  for (auto i = 0U; i < iterations; ++i) {
    result |= (result >> bit_width);
  }
  return result;
}
}  // namespace hal::inline v6
