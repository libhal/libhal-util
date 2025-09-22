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

#include <type_traits>

#include <libhal/units.hpp>

/**
 * @defgroup Enum Enums
 *
 */

namespace hal {
/**
 * @ingroup Enum
 * @brief concept for enumeration types
 *
 * @tparam T - enum type
 */
template<typename T>
concept enumeration = std::is_enum_v<T>;

/**
 * @ingroup Enum
 * @brief Helper function to convert an enum to its integral value
 *
 * @param p_enum_value - the enumeration you want to convert into an integral
 * value
 * @return constexpr auto - return the integral value of the enum with the same
 * type as the enumeration.
 */
[[nodiscard]] constexpr auto value(enumeration auto p_enum_value)
{
  return static_cast<std::underlying_type_t<decltype(p_enum_value)>>(
    p_enum_value);
}
}  // namespace hal
