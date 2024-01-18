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

#include <libhal/input_pin.hpp>

/**
 * @defgroup InputPin Input Pin
 *
 */
namespace hal {
/**
 * @ingroup InputPin
 * @brief Compares two input pin states
 *
 * @param p_lhs An input pin
 * @param p_rhs An input pin
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const input_pin::settings& p_lhs,
                                        const input_pin::settings& p_rhs)
{
  return p_lhs.resistor == p_rhs.resistor;
}
}  // namespace hal
