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

#include <libhal/timer.hpp>

namespace hal {
/**
 * @brief Inert implementation of timer hardware
 *
 */
class inert_timer : public hal::timer
{
public:
  /**
   * @brief Create inert_timer object
   *
   * @param p_is_running - what will be returned from the is_running function.
   */
  constexpr inert_timer(bool p_is_running)
    : m_is_running(p_is_running)
  {
  }

private:
  bool driver_is_running()
  {
    return m_is_running;
  }

  void driver_cancel()
  {
  }
  void driver_schedule([[maybe_unused]] hal::callback<void(void)> p_callback,
                       [[maybe_unused]] hal::time_duration p_delay)
  {
  }

  bool m_is_running;
};
}  // namespace hal
