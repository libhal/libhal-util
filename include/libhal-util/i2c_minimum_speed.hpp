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

#include <libhal/i2c.hpp>

namespace hal {

// TODO(#38) Remove on libhal 5.0.0
/**
 * @brief A i2c wrapper to ensure that the lowest i2c device frequency is used.
 * @deprecated Do not use this class as it is a trap. This wrapper cannot ensure
 * that the i2c clock speed is the minimum possible for all devices on the bus
 * without knowledge about the system before hand.
 */
class minimum_speed_i2c : public hal::i2c
{
public:
  constexpr static auto default_max_speed = 2'000'000;
  /**
   * @brief Create minimum_speed_i2c object.
   *
   * @param p_i2c - i2c object that the device will use
   * @param p_frequency - the maximum starting frequency the device can use
   */
  minimum_speed_i2c(hal::i2c& p_i2c, hertz p_frequency = default_max_speed);

private:
  /**
   * @brief Pass through configuration function from this class to the passed
   * i2c driver.
   *
   * @param p_new_setting - settings to be set
   */
  void driver_configure(settings const& p_new_setting) override;

  void driver_transaction(
    hal::byte p_address,
    std::span<hal::byte const> p_data_out,
    std::span<hal::byte> p_data_in,
    hal::function_ref<hal::timeout_function> p_timeout) override;

  hal::i2c* m_i2c;
  hertz m_lowest_seen_frequency;
};
}  // namespace hal
