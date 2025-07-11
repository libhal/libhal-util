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

#include <libhal/motor.hpp>

namespace hal {
/**
 * @brief Inert implementation of open loop motorized actuator hardware
 *
 */
class inert_motor : public hal::motor
{
public:
  /**
   * @brief Create inert_motor object
   *
   */
  constexpr inert_motor()
  {
  }

private:
  void driver_power([[maybe_unused]] float p_power)
  {
  }
};
}  // namespace hal
