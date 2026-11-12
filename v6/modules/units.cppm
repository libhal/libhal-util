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

module;

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ios>
#include <limits>

export module hal.util:units;

import hal;
import :math;

/**
 * @defgroup UnitsUtils Units Utils
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup UnitsUtils
 * @brief Calculate the number of cycles of this frequency within the time
 * duration. This function is meant for timers to determine how many count
 * cycles are needed to reach a particular time duration at this frequency.
 *
 * The result rounds up, so the cycles never take less than the duration, as a
 * timer's delay must not. A positive duration shorter than one cycle gives 1.
 *
 * @param p_source - source frequency
 * @param p_duration - the amount of time to convert to cycles
 * @return hal::u64 - number of cycles: 0 if `p_duration` isn't positive, and
 * the largest `hal::u64` if it is 2^32 seconds (about 136 years) or more
 */
export [[nodiscard]] constexpr hal::u64 cycles_per(
  hertz p_source,
  hal::time_duration p_duration)
{
  // Full Equation:
  //                              / ratio_num \_
  //   frequency_hz * |period| * | ----------- |  = cycles
  //                              \ ratio_den /
  //
  // hal::time_duration::period::num == 1
  // hal::time_duration::period::den == 1,000,000,000
  using period = hal::time_duration::period;
  static_assert(period::num == 1, "The period ratio numerator must be 1");
  constexpr hal::u64 counts_per_second = period::den;
  constexpr hal::u64 max_seconds = std::numeric_limits<hal::u32>::max();

  if (p_duration <= hal::time_duration::zero()) {
    return 0;
  }

  // Multiplying the whole duration by the frequency overflows 64 bits after
  // about 9 seconds at 1 GHz. Whole seconds times the frequency, plus the
  // cycles in the rest of a second, can't: both the seconds and the frequency
  // fit in 32 bits.
  hal::u64 const frequency = p_source.numerical_value_in(hertz::unit);
  auto const counts = static_cast<hal::u64>(p_duration.count());
  auto const seconds = counts / counts_per_second;
  auto const rest = counts % counts_per_second;
  if (seconds > max_seconds) {
    return std::numeric_limits<hal::u64>::max();
  }
  auto const rest_cycles =
    ((rest * frequency) + (counts_per_second - 1)) / counts_per_second;
  return (seconds * frequency) + rest_cycles;
}

/**
 * @ingroup UnitsUtils
 * @brief Calculates and returns the wavelength in seconds.
 *
 * @tparam Period - desired period (defaults to std::femto for femtoseconds).
 * @param p_source - source frequency to convert to wavelength
 * @return std::chrono::duration<hal::u32, Period> - time based wavelength of
 * the frequency.
 */
export template<typename Period = hal::time_duration::period>
constexpr auto wavelength(hertz p_source)
{
  using rep = hal::u64;
  using duration = std::chrono::duration<rep, Period>;

  static_assert(Period::num == 1, "The period ratio numerator must be 1");
  static_assert(Period::den >= 1,
                "The period ratio denominator must be 1 or greater than 1.");

  auto const source_hz =
    static_cast<float>(p_source.numerical_value_in(hertz::unit));
  constexpr auto denominator = static_cast<float>(Period::den);
  auto period = (1.0f / source_hz) * denominator;

  if (std::isinf(period)) {
    return duration(std::numeric_limits<hal::u32>::max());
  }

  return duration(static_cast<hal::u32>(period));
}

/**
 * @ingroup UnitsUtils
 * @brief Calculates and returns the wavelength in seconds as a float.
 *
 * @tparam float_t - float type
 * @tparam Period - desired period
 * @param p_source - source frequency to convert to wavelength
 * @return constexpr float - float representation of the time based wavelength
 * of the frequency.
 */
export constexpr float wavelength(hertz p_source)
{
  auto const source_hz =
    static_cast<float>(p_source.numerical_value_in(hertz::unit));
  if (equals(source_hz, 0.0f)) {
    return 0.0f;
  }
  return 1.0f / source_hz;
}

/**
 * @ingroup UnitsUtils
 * @brief Calculate how long a number of cycles of a frequency takes
 *
 * The reverse of `hal::cycles_per()`. For code that counts in ticks of a
 * clock, such as a deadline on a steady clock, and needs the time left as a
 * duration, for example to sleep until the deadline.
 *
 * The result rounds up, so the duration is never shorter than the cycles
 * take, as a sleep until a deadline must not be.
 *
 * @param p_source - the frequency of the cycles
 * @param p_cycles - the number of cycles
 * @return hal::time_duration - the time the cycles take: 0 for no cycles, and
 * `hal::time_duration::max()` (about 292 years) if they take its last second
 * or longer, or if `p_source` is 0 Hz, which never completes a cycle.
 */
export [[nodiscard]] constexpr hal::time_duration duration_from_cycles(
  hertz p_source,
  hal::u64 p_cycles)
{
  // Full Equation (the reverse of cycles_per()):
  //
  //                    cycles * ratio_den
  //   duration =  --------------------------
  //                frequency_hz * ratio_num
  //
  // hal::time_duration::period::num == 1
  // hal::time_duration::period::den == 1,000,000,000
  using period = hal::time_duration::period;
  static_assert(period::num == 1, "The period ratio numerator must be 1");
  constexpr hal::u64 counts_per_second = period::den;
  constexpr auto max_counts =
    static_cast<hal::u64>(hal::time_duration::max().count());

  hal::u64 const frequency = p_source.numerical_value_in(hertz::unit);
  if (frequency == 0) {
    return (p_cycles == 0) ? hal::time_duration::zero()
                           : hal::time_duration::max();
  }

  // Multiplying all the cycles by a billion overflows 64 bits past about 18
  // billion cycles. Whole seconds, plus the counts in the cycles left over,
  // can't: fewer cycles are left over than the frequency, which fits in 32
  // bits.
  auto const seconds = p_cycles / frequency;
  auto const rest = p_cycles % frequency;
  // From the last whole second that fits on, the duration could pass max().
  // max() itself is never shorter than the cycles take.
  if (seconds >= (max_counts / counts_per_second)) {
    return hal::time_duration::max();
  }
  auto const rest_counts =
    ((rest * counts_per_second) + (frequency - 1)) / frequency;
  return hal::time_duration(static_cast<hal::time_duration::rep>(
    (seconds * counts_per_second) + rest_counts));
}

/**
 * @ingroup UnitsUtils
 * @brief print byte type using ostreams
 *
 * Meant for unit testing, testing and simulation purposes
 * C++ streams, in general, should not be used for any embedded project that
 * will ever have to be used on an MCU due to its memory cost.
 *
 * @tparam CharT - character type
 * @tparam Traits - ostream traits type
 * @param p_ostream - the ostream
 * @param p_byte - object to convert to a string
 * @return std::basic_ostream<CharT, Traits>& - reference to the ostream
 */
export template<class CharT, class Traits>
inline std::basic_ostream<CharT, Traits>& operator<<(
  std::basic_ostream<CharT, Traits>& p_ostream,
  hal::byte const& p_byte)
{
  return p_ostream << std::hex << "0x" << static_cast<unsigned>(p_byte);
}
}  // namespace hal::inline v6
