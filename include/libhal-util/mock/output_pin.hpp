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

#include <libhal/output_pin.hpp>

#include "testing.hpp"

namespace hal {
/**
 * @brief mock output pin for use in unit tests and simulations
 *
 */
struct mock_output_pin : public hal::output_pin
{
  /**
   * @brief Reset spy information for configure() and level()
   *
   */
  void reset()
  {
    spy_configure.reset();
    spy_level.reset();
  }

  /// Spy handler for hal::output_pin::configure()
  spy_handler<settings> spy_configure;
  /// Spy handler for hal::output_pin::level()
  spy_handler<bool> spy_level;

private:
  void driver_configure(settings const& p_settings) override
  {
    spy_configure.record(p_settings);
  }
  void driver_level(bool p_high) override
  {
    m_current_level = p_high;
    spy_level.record(m_current_level);
  }
  bool driver_level() override
  {
    return m_current_level;
  }

  bool m_current_level{ false };
};
}  // namespace hal
