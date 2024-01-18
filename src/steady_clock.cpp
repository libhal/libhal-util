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

#include <libhal-util/steady_clock.hpp>

namespace hal {
std::uint64_t future_deadline(hal::steady_clock& p_steady_clock,
                              hal::time_duration p_duration)
{
  using period = decltype(p_duration)::period;
  const auto frequency = p_steady_clock.frequency().operating_frequency;
  const auto tick_period = wavelength<period>(frequency);
  auto ticks_required = p_duration / tick_period;
  using unsigned_ticks = std::make_unsigned_t<decltype(ticks_required)>;

  if (ticks_required <= 1) {
    ticks_required = 1;
  }

  const auto ticks = static_cast<unsigned_ticks>(ticks_required);
  const auto future_timestamp = ticks + p_steady_clock.uptime().ticks;

  return future_timestamp;
}
steady_clock_timeout steady_clock_timeout::create(
  hal::steady_clock& p_steady_clock,
  hal::time_duration p_duration)
{
  const auto deadline = future_deadline(p_steady_clock, p_duration);
  return { p_steady_clock, deadline };
}

status steady_clock_timeout::operator()()
{
  auto current_count = m_counter->uptime().ticks;

  if (current_count >= m_cycles_until_timeout) {
    return hal::new_error(std::errc::timed_out);
  }

  return success();
}

steady_clock_timeout::steady_clock_timeout(hal::steady_clock& p_steady_clock,
                                           std::uint64_t p_cycles_until_timeout)
  : m_counter(&p_steady_clock)
  , m_cycles_until_timeout(p_cycles_until_timeout)
{
}

steady_clock_timeout create_timeout(hal::steady_clock& p_steady_clock,
                                    hal::time_duration p_duration)
{
  return steady_clock_timeout::create(p_steady_clock, p_duration);
}

void delay(hal::steady_clock& p_steady_clock, hal::time_duration p_duration)
{
  auto ticks_until_timeout = future_deadline(p_steady_clock, p_duration);
  while (p_steady_clock.uptime().ticks < ticks_until_timeout) {
    continue;
  }
}
}  // namespace hal
