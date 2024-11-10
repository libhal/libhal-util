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

#include <libhal/steady_clock.hpp>

namespace hal {
/**
 * @brief Inert implementation of steady clock mechanism
 *
 */
class inert_steady_clock : public hal::steady_clock
{
public:
  /**
   * @brief Create inert_steady_clock object
   *
   * @param p_frequency - what will be returned from the frequency function.
   * @param p_uptime - the starting value that will be returned when uptime() is
   * called. The uptime will increment by 1 each time uptime() is called.
   */
  constexpr inert_steady_clock(hal::hertz p_frequency, std::uint64_t p_uptime)
    : m_frequency(p_frequency)
    , m_uptime(p_uptime)
  {
  }

private:
  hal::hertz driver_frequency()
  {
    return m_frequency;
  }

  std::uint64_t driver_uptime()
  {
    m_uptime++;
    return m_uptime;
  }

  hal::hertz m_frequency;
  std::uint64_t m_uptime;
};
}  // namespace hal
