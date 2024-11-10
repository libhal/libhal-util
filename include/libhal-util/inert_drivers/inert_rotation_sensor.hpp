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

#include <libhal/rotation_sensor.hpp>

namespace hal {
/**
 * @brief Inert implementation of rotation measuring hardware
 *
 */
class inert_rotation_sensor : public hal::rotation_sensor
{
public:
  /**
   * @brief Create inert_rotation_sensor object
   *
   * @param p_position - what will be returned from the read function.
   */
  constexpr inert_rotation_sensor(read_t p_position)
    : m_position(p_position)
  {
  }

private:
  read_t driver_read()
  {
    return m_position;
  }

  read_t m_position;
};
}  // namespace hal
