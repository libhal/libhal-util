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
#include <limits>

export module hal.util:steady_clock;

import hal;

/**
 * @defgroup SteadyClock Steady Clock
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup SteadyClock
 * @brief The ticks to add to a steady clock's uptime for a deadline at least a
 * duration away
 *
 * For code that keeps a timeout as a number of ticks and adds it to
 * `uptime()` each time it starts waiting. The duration is converted to ticks
 * of `p_frequency`, rounded up, plus one more, since part of the tick in
 * progress has already passed when the deadline is set. So a deadline of
 * `uptime()` plus these ticks is never reached sooner than `p_duration` later.
 *
 * @param p_frequency - the frequency of the steady clock
 * @param p_duration - the least time until the deadline
 * @return hal::u64 - the ticks to add to `uptime()`: 1, the next tick, if
 * `p_duration` isn't positive, and the largest `hal::u64` if it is 2^32
 * seconds (about 136 years) or more.
 */
export [[nodiscard]] constexpr hal::u64 deadline_ticks(
  hertz p_frequency,
  hal::time_duration p_duration)
{
  using period = hal::time_duration::period;
  static_assert(period::num == 1, "The period ratio numerator must be 1");
  constexpr hal::u64 counts_per_second = period::den;
  constexpr hal::u64 max_seconds = std::numeric_limits<hal::u32>::max();
  constexpr hal::u64 max_ticks = std::numeric_limits<hal::u64>::max();

  if (p_duration <= hal::time_duration::zero()) {
    return 1;
  }

  // Whole seconds times the frequency, plus the ticks in the rest of a second,
  // rounded up. Both the seconds and the frequency fit in 32 bits, so every
  // product and sum fits in 64 bits.
  hal::u64 const frequency = p_frequency.numerical_value_in(hertz::unit);
  auto const counts = static_cast<hal::u64>(p_duration.count());
  auto const seconds = counts / counts_per_second;
  auto const rest = counts % counts_per_second;
  if (seconds > max_seconds) {
    return max_ticks;
  }
  auto const rest_ticks =
    ((rest * frequency) + (counts_per_second - 1)) / counts_per_second;
  return (seconds * frequency) + rest_ticks + 1;
}

/**
 * @ingroup SteadyClock
 * @brief Compute a deadline at least a duration from now
 *
 * The deadline is in ticks of `p_steady_clock`: `uptime()` plus
 * `deadline_ticks()` of the duration. Once `uptime()` reaches it, at least
 * `p_duration` has passed.
 *
 * @param p_steady_clock - the steady clock to compute the deadline for. The
 * deadline only works with this clock.
 * @param p_duration - the least time until the deadline
 * @return hal::u64 - the deadline in ticks of `p_steady_clock`. A duration
 * that isn't positive gives the next tick. A deadline past what 64 bits can
 * count gives the largest `hal::u64`, which the clock never reaches.
 */
export hal::u64 future_deadline(hal::steady_clock& p_steady_clock,
                                hal::time_duration p_duration)
{
  constexpr hal::u64 max_deadline = std::numeric_limits<hal::u64>::max();
  auto const ticks = deadline_ticks(p_steady_clock.frequency(), p_duration);
  auto const now = p_steady_clock.uptime();
  if (ticks > (max_deadline - now)) {
    return max_deadline;
  }
  return now + ticks;
}
}  // namespace hal::inline v6
