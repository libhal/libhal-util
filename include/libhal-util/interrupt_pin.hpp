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

/**
 * @defgroup InterruptPin Interrupt Pin
 *
 */
namespace hal {
/**
 * @ingroup InterruptPin
 * @brief Compares two interrupt pin states, compares both their trigger and
 * resistor.
 *
 * @param p_lhs An interrupt pin
 * @param p_rhs An interrupt pin
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const interrupt_pin::settings& p_lhs,
                                        const interrupt_pin::settings& p_rhs)
{
  return p_lhs.resistor == p_rhs.resistor && p_lhs.trigger == p_rhs.trigger;
}
}  // namespace hal
