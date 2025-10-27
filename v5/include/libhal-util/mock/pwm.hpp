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

#include <libhal/pwm.hpp>

#include "testing.hpp"

namespace hal {
/**
 * @brief Mock pwm implementation for use in unit tests and simulations with spy
 * functions for frequency() and duty_cycle().
 *
 */
struct mock_pwm : public hal::pwm
{
  /**
   * @brief Reset spy information for both frequency() and duty_cycle()
   *
   */
  void reset()
  {
    spy_frequency.reset();
    spy_duty_cycle.reset();
  }

  /// Spy handler for hal::pwm::frequency()
  spy_handler<hertz> spy_frequency;
  /// Spy handler for hal::pwm::duty_cycle()
  spy_handler<float> spy_duty_cycle;

private:
  void driver_frequency(hertz p_settings) override
  {
    spy_frequency.record(p_settings);
  }

  void driver_duty_cycle(float p_duty_cycle) override
  {
    spy_duty_cycle.record(p_duty_cycle);
  }
};
}  // namespace hal
