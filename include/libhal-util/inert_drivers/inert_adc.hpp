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

#include <libhal/adc.hpp>

#include <algorithm>

namespace hal {
/**
 * @brief Inert implementation of Analog to Digital Converter (ADC) hardware
 *
 */
class inert_adc : public hal::adc
{
public:
  /**
   * @brief Create inert_adc object
   *
   * @param p_result - What will be returned from inert_adc's read function,
   * clamped to -1.0f to 1.0f.
   */
  constexpr inert_adc(float p_result)
    : m_result(std::clamp(p_result, -1.0f, 1.0f))
  {
  }

private:
  float driver_read()
  {
    return m_result;
  }

  float m_result;
};
}  // namespace hal
