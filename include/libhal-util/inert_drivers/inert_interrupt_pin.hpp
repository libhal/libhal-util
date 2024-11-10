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

namespace hal {
/**
 * @brief Inert implementation of digital interrupt pin hardware
 *
 */
class inert_interrupt_pin : public hal::interrupt_pin
{
public:
  /**
   * @brief Create inert_interrupt_pin object
   */
  constexpr inert_interrupt_pin()
  {
  }

private:
  void driver_configure([[maybe_unused]] settings const& p_settings)
  {
  }
  void driver_on_trigger([[maybe_unused]] hal::callback<handler> p_callback)
  {
  }
};
}  // namespace hal
