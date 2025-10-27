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

#include <libhal/input_pin.hpp>

namespace hal {
/**
 * @brief Inert implementation of digital input pin hardware
 *
 */
class inert_input_pin : public hal::input_pin
{
public:
  /**
   * @brief Create inert_input_pin object
   *
   * @param p_level - what will be returned from the level function.
   */
  constexpr inert_input_pin(bool p_level)
    : m_level(p_level)
  {
  }

private:
  void driver_configure([[maybe_unused]] settings const& p_settings)
  {
  }

  bool driver_level()
  {
    return m_level;
  }

  bool m_level;
};
}  // namespace hal
