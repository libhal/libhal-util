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

#include <libhal/error.hpp>
#include <libhal/steady_clock.hpp>
#include <libhal/timeout.hpp>

#include "units.hpp"

/**
 * @defgroup SteadyClock Steady Clock
 *
 */

namespace hal {
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
inline std::uint64_t future_deadline(hal::steady_clock& p_steady_clock,
                                     hal::time_duration p_duration);

/**
 * @ingroup SteadyClock
 * @brief Timeout object based on hal::steady_clock
 *
 * Do not use this class directly. Use `hal::create_timeout(hal::steady_clock&)`
 * instead of instantiating this class directly.
 *
 */
class steady_clock_timeout
{
public:
  /**
   * @ingroup SteadyClock
   * @brief Create a steady_clock_timeout
   *
   * @return steady_clock_timeout - steady_clock_timeout object
   */
  static steady_clock_timeout create(hal::steady_clock& p_steady_clock,
                                     hal::time_duration p_duration);

  /**
   * @ingroup SteadyClock
   * @brief Construct a new counter timeout object
   *
   * @param p_timeout - other steady_clock_timeout
   */
  steady_clock_timeout(const steady_clock_timeout& p_timeout) = default;
  /**
   * @ingroup SteadyClock
   * @brief Assign construct a new counter timeout object
   *
   * @param p_timeout - other steady_clock_timeout
   * @return steady_clock_timeout&
   */
  steady_clock_timeout& operator=(const steady_clock_timeout& p_timeout) =
    default;
  /**
   * @ingroup SteadyClock
   * @brief Construct a new counter timeout object
   *
   * @param p_timeout - other steady_clock_timeout
   */
  steady_clock_timeout(steady_clock_timeout&& p_timeout) = default;
  /**
   * @ingroup SteadyClock
   * @brief Move assign construct a new counter timeout object
   *
   * @param p_timeout - other steady_clock_timeout
   * @return steady_clock_timeout&
   */
  steady_clock_timeout& operator=(steady_clock_timeout&& p_timeout) = default;

  /**
   * @ingroup SteadyClock
   * @brief Call this object to check if it has timed out.
   *
   * @return status - success or failure
   * @throws std::errc::timed_out - if the timeout time has been exceeded.
   */
  status operator()();

private:
  /**
   * @ingroup SteadyClock
   * @brief Construct a new counter timeout object
   *
   * @param p_steady_clock - steady clock implementation
   * @param p_cycles_until_timeout - number of cycles until timeout
   */
  steady_clock_timeout(hal::steady_clock& p_steady_clock,
                       std::uint64_t p_cycles_until_timeout);

  hal::steady_clock* m_counter;
  std::uint64_t m_cycles_until_timeout = 0;
};

/**
 * @ingroup SteadyClock
 * @brief Create a timeout object based on hal::steady_clock.
 *
 * NOTE: Multiple timeout objects can be made from a single steady_clock
 * without influencing other timeout objects.
 *
 * @param p_steady_clock - hal::steady_clock implementation
 * @param p_duration - amount of time until timeout
 * @return hal::steady_clock_timeout - timeout object
 */
steady_clock_timeout create_timeout(hal::steady_clock& p_steady_clock,
                                    hal::time_duration p_duration);

/**
 * @ingroup SteadyClock
 * @brief Delay execution for a duration of time using a hardware steady_clock.
 *
 * @param p_steady_clock - steady_clock driver
 * @param p_duration - the amount of time to delay for. Zero or negative time
 * duration will delay for one tick of the p_steady_clock.
 */
void delay(hal::steady_clock& p_steady_clock, hal::time_duration p_duration);

/**
 * @ingroup SteadyClock
 * @brief Generates a function that, when passed a duration, returns a timeout
 *
 * @param p_steady_clock - steady_clock driver that must out live the lifetime
 * of the returned lambda.
 * @return auto - a callable that returns a new timeout object each time a time
 * duration is passed to it.
 */
inline auto timeout_generator(hal::steady_clock& p_steady_clock)
{
  return [&p_steady_clock](hal::time_duration p_duration) -> auto
  {
    return create_timeout(p_steady_clock, p_duration);
  };
}
}  // namespace hal
