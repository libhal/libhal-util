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

#include <queue>

#include <libhal/adc.hpp>
#include <libhal/error.hpp>

namespace hal {
/**
 * @brief Mock adc implementation for use in unit tests and simulations.
 */
struct mock_adc : public hal::adc
{
  /**
   * @brief Queues the floats to be returned for read()
   *
   * @param p_adc_values - queue of floats
   */
  void set(std::queue<float>& p_adc_values)
  {
    m_adc_values = p_adc_values;
  }

private:
  /**
   * @brief mock implementation of driver_read()
   *
   * @return float - adc value from queue
   * @throws throw hal::operation_not_permitted - if the adc queue runs out
   */
  float driver_read() override
  {
    if (m_adc_values.size() == 0) {
      throw hal::operation_not_permitted(this);
    }
    auto m_current_value = m_adc_values.front();
    m_adc_values.pop();
    return m_current_value;
  }

  std::queue<float> m_adc_values{};
};
}  // namespace hal
