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

#include <libhal-util/steady_clock.hpp>
#include <libhal/error.hpp>

namespace hal {
std::uint64_t future_deadline(hal::steady_clock& p_steady_clock,
                              hal::time_duration p_duration)
{
  using period = decltype(p_duration)::period;
  auto const frequency = p_steady_clock.frequency();
  auto const tick_period = wavelength<period>(frequency);
  auto ticks_required = p_duration / tick_period;
  using unsigned_ticks = std::make_unsigned_t<decltype(ticks_required)>;

  if (ticks_required <= 1) {
    ticks_required = 1;
  }

  auto const ticks = static_cast<unsigned_ticks>(ticks_required);
  auto const future_timestamp = ticks + p_steady_clock.uptime();

  return future_timestamp;
}

void steady_clock_timeout::operator()()
{
  auto current_count = m_counter->uptime();
  if (current_count >= m_cycles_until_timeout) {
    hal::safe_throw(hal::timed_out(this));
  }
}

steady_clock_timeout::steady_clock_timeout(hal::steady_clock& p_steady_clock,
                                           hal::time_duration p_duration)
  : m_counter(&p_steady_clock)
  , m_cycles_until_timeout(future_deadline(p_steady_clock, p_duration))
{
}

steady_clock_timeout create_timeout(hal::steady_clock& p_steady_clock,
                                    hal::time_duration p_duration)
{
  return { p_steady_clock, p_duration };
}

void delay(hal::steady_clock& p_steady_clock, hal::time_duration p_duration)
{
  auto ticks_until_timeout = future_deadline(p_steady_clock, p_duration);
  while (p_steady_clock.uptime() < ticks_until_timeout) {
    continue;
  }
}
}  // namespace hal
