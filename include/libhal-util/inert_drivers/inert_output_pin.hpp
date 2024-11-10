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

namespace hal {
/**
 * @brief Inert implementation of digital output pin hardware
 *
 */
class inert_output_pin : public hal::output_pin
{
public:
  /**
   * @brief Create inert_output_pin
   *
   * @param p_level - level_t object to return when level() is called. This
   * value can be changed by using level(bool) after creation.
   */
  constexpr inert_output_pin(bool p_level)
    : m_level(p_level)
  {
  }

private:
  void driver_configure([[maybe_unused]] settings const& p_settings)
  {
  }
  void driver_level(bool p_high)
  {
    m_level = p_high;
  }

  bool driver_level()
  {
    return m_level;
  }

  bool m_level;
};

/**
 * @brief Returns a default inert output pin for use as a default parameter
 *
 * Consider a driver that can accept an output pin but normally doesn't need to
 * use it, this paramater can be used as a default parameters. For example, a
 * driver that can optionally light up a status LED during activity. Rather than
 * use std::optional which would add an additional branch at all unique call
 * sites for the output pin, adding to code size, this driver can be used as a
 * substitute.
 *
 * @return hal::output_pin& - reference to a single default inert output pin.
 * All calls to this function return the same output pin.
 */
inline hal::output_pin& default_inert_output_pin()
{
  static inert_output_pin _default_inert_output_pin(false);
  return _default_inert_output_pin;
}
}  // namespace hal
