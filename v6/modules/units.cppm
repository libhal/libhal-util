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
#include <optional>

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
 * @param p_source - source frequency
 * @param p_duration - the amount of time to convert to cycles
 * @return hal::u32 - number of cycles
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
  // hal::time_duration::period::den == 1,000,000

  auto const denominator = decltype(p_duration)::period::den;
  auto const count = p_duration.count();
  auto const source_hz = p_source.numerical_value_in(hertz::unit);
  auto const cycle_count = (count * source_hz) / denominator;

  return static_cast<hal::u64>(cycle_count);
}

/**
 * @ingroup UnitsUtils
 * @brief Calculate the fewest polls that take at least a duration
 *
 * For a loop that gives up after a number of polls instead of reading a
 * clock. Each poll must take at least `p_cycles_per_poll` cycles of
 * `p_source`, for example the bus cycles that reading a register takes. The
 * count rounds up, and polls that take longer only make the loop longer, so
 * the polls never take less than `p_duration`.
 *
 * @param p_source - the clock that times each poll
 * @param p_duration - how long the polls must take at least
 * @param p_cycles_per_poll - the fewest cycles of `p_source` a poll takes
 * @return std::optional<hal::u32> - the number of polls, 0 if `p_duration`
 * isn't positive, or std::nullopt if `p_source` or `p_cycles_per_poll` is
 * zero, the number doesn't fit in 32 bits, or `p_duration` is 2^32 seconds
 * (about 136 years) or longer.
 */
export [[nodiscard]] constexpr std::optional<hal::u32> polls_for(
  hertz p_source,
  hal::time_duration p_duration,
  hal::u32 p_cycles_per_poll)
{
  using period = hal::time_duration::period;
  static_assert(period::num == 1, "The period ratio numerator must be 1");
  constexpr hal::u64 ticks_per_second = period::den;
  constexpr hal::u64 max_polls = std::numeric_limits<hal::u32>::max();

  hal::u64 const frequency = p_source.numerical_value_in(hertz::unit);
  hal::u64 const cycles_per_poll = p_cycles_per_poll;
  if ((frequency == 0) or (cycles_per_poll == 0)) {
    return std::nullopt;
  }
  if (p_duration <= hal::time_duration::zero()) {
    return 0;
  }

  // The cycles the polls must take, rounded up: whole seconds times the
  // frequency, plus the cycles in the rest of a second. Splitting the
  // duration this way keeps every product within 64 bits, for fewer than
  // 2^32 seconds.
  auto const ticks = static_cast<hal::u64>(p_duration.count());
  auto const seconds = ticks / ticks_per_second;
  auto const rest = ticks % ticks_per_second;
  if (seconds > max_polls) {
    return std::nullopt;
  }
  auto const rest_cycles =
    ((rest * frequency) + (ticks_per_second - 1)) / ticks_per_second;
  auto const cycles = (seconds * frequency) + rest_cycles;
  if (cycles > (max_polls * cycles_per_poll)) {
    return std::nullopt;
  }
  return static_cast<hal::u32>((cycles + (cycles_per_poll - 1)) /
                               cycles_per_poll);
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
  auto duration = (1.0f / source_hz);
  return float(duration);
}

/**
 * @ingroup UnitsUtils
 * @brief Calculate the amount of time it takes a frequency to oscillate a
 * number of cycles.
 *
 * @param p_source - the frequency to compute the cycles from
 * @param p_cycles - number of cycles within the time duration
 * @return std::optional<hal::time_duration> - time duration based on this
 * frequency and the number of cycles. Will return std::nullopt if the duration
 * exceeds
 */
export [[nodiscard]] inline std::optional<hal::time_duration>
duration_from_cycles(hertz p_source, uint32_t p_cycles)
{
  // Full Equation (based on the equation in cycles_per()):
  //
  //
  //                /    cycles * ratio_den    \_
  //   |period| =  | ---------------------------|
  //                \ frequency_hz * ratio_num /
  //
  constexpr auto ratio_den =
    static_cast<float>(hal::time_duration::period::den);
  constexpr auto ratio_num = hal::time_duration::period::num;
  constexpr auto int_min = std::numeric_limits<hal::time_duration::rep>::min();
  constexpr auto int_max = std::numeric_limits<hal::time_duration::rep>::max();
  constexpr auto float_int_min = static_cast<float>(int_min);
  constexpr auto float_int_max = static_cast<float>(int_max);

  auto const source_hz =
    static_cast<float>(p_source.numerical_value_in(hertz::unit));
  auto const source = std::abs(source_hz);
  auto const float_cycles = static_cast<float>(p_cycles);
  auto const nanoseconds = (float_cycles * ratio_den) / (source * ratio_num);

  if (float_int_min <= nanoseconds && nanoseconds <= float_int_max) {
    return hal::time_duration(
      static_cast<hal::time_duration::rep>(nanoseconds));
  }

  return std::nullopt;
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
  return p_ostream << std::hex << "0x" << unsigned(p_byte);
}
}  // namespace hal::inline v6
