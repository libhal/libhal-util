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

#include <libhal/interrupt_pin.hpp>

#include "testing.hpp"

namespace hal {
/**
 * @brief mock interrupt_pin implementation for use in unit tests and
 * simulations.
 *
 */
struct mock_interrupt_pin : public hal::interrupt_pin
{
  /**
   * @brief Reset spy information for configure(), on_trigger(), and
   * disable()
   *
   */
  void reset()
  {
    spy_configure.reset();
    spy_on_trigger.reset();
  }

  /// Spy handler for hal::interrupt_pin::configure()
  spy_handler<settings> spy_configure;
  /// Spy handler for hal::interrupt_pin::on_trigger()
  spy_handler<std::function<handler>> spy_on_trigger;

private:
  void driver_configure(settings const& p_settings) override
  {
    spy_configure.record(p_settings);
  }
  void driver_on_trigger(hal::callback<handler> p_callback) override
  {
    spy_on_trigger.record(p_callback);
  }
};
}  // namespace hal
