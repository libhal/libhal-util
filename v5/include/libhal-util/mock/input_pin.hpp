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

#include <libhal/error.hpp>
#include <queue>

#include <libhal/input_pin.hpp>

#include "testing.hpp"

namespace hal {
/**
 * @brief mock input_pin implementation for use in unit tests and simulations.
 *
 */
struct mock_input_pin : public hal::input_pin
{
  /**
   * @brief Reset spy information for configure()
   *
   */
  void reset()
  {
    spy_configure.reset();
  }

  /// Spy handler for embed:input_pin::configure()
  spy_handler<settings> spy_configure;

  /**
   * @brief Queues the active levels to be returned for levels()
   *
   * @param p_levels - queue of actives levels
   */
  void set(std::queue<bool>& p_levels)
  {
    m_levels = p_levels;
  }

private:
  void driver_configure(settings const& p_settings) override
  {
    spy_configure.record(p_settings);
  }
  /**
   * @brief Mock implementation of input_pin::driver_level
   *
   * @return true - high voltage
   * @return false - low voltage
   * @throws throw hal::operation_not_permitted - if the input pin value queue
   * runs out of elements
   */
  bool driver_level() override
  {
    // This comparison performs bounds checking because front() and pop() do
    // not bounds check and results in undefined behavior if the queue is empty.
    if (m_levels.size() == 0) {
      throw hal::operation_not_permitted(this);
    }
    auto m_current_value = m_levels.front();
    m_levels.pop();
    return m_current_value;
  }

  std::queue<bool> m_levels{};
};
}  // namespace hal
