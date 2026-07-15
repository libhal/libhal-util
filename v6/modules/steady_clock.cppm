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

#include <coroutine>
#include <type_traits>

export module hal.util:steady_clock;

import hal;

import :units;

/**
 * @defgroup SteadyClock Steady Clock
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup SteadyClock
 * @brief Function to compute a future timestamp in ticks
 *
 * This function calculates a future timestamp based on the current uptime of a
 * steady clock and a specified duration.
 *
 * @param p_steady_clock - the steady_clock used to calculate the future
 * duration. Note that this future deadline will only work with this steady
 * clock.
 * @param p_duration The duration for which we need to compute a future
 * timestamp.
 *
 * @return A 64-bit unsigned integer representing the future timestamp in steady
 * clock ticks. The future timestamp is calculated as the sum of the current
 * number of ticks of the clock and the number of ticks equivalent to the
 * specified duration. If the duration corresponds to a ticks_required value
 * less than or equal to 1, it will be set to 1 to ensure at least one tick is
 * waited.
 */
export async::future<hal::u64> future_deadline(
  async::context& p_ctx,
  hal::steady_clock& p_steady_clock,
  hal::time_duration p_duration)
{

  using period = decltype(p_duration)::period;
  auto const frequency = co_await p_steady_clock.frequency(p_ctx);
  auto const tick_period = wavelength<period>(frequency);
  auto ticks_required = p_duration.count() / tick_period.count();
  using unsigned_ticks = std::make_unsigned_t<decltype(ticks_required)>;

  if (ticks_required <= 1) {
    ticks_required = 1;
  }

  auto const ticks = static_cast<unsigned_ticks>(ticks_required);
  auto const future_timestamp = ticks + co_await p_steady_clock.uptime(p_ctx);

  co_return future_timestamp;
}
}  // namespace hal::inline v6
