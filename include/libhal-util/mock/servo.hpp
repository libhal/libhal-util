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

#include <libhal/servo.hpp>

#include "testing.hpp"

namespace hal {
/**
 * @brief Mock servo implementation for use in unit tests and simulations with a
 * spy function for position()
 *
 */
struct mock_servo : public hal::servo
{
  /**
   * @brief Reset spy information for position()
   *
   */
  void reset()
  {
    spy_position.reset();
  }

  /// Spy handler for hal::servo::position()
  spy_handler<float> spy_position;

private:
  void driver_position(float p_position) override
  {
    spy_position.record(p_position);
  };
};
}  // namespace hal
