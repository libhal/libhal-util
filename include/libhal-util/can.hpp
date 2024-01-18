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

#include <cmath>

#include <libhal/can.hpp>

#include "comparison.hpp"
#include "math.hpp"

/**
 * @defgroup CAN_Utilities CAN Utilities
 *
 */

namespace hal {
/**
 * @ingroup CAN_Utilities
 * @brief Compares two CAN bus states.
 *
 * @param p_lhs A CAN bus.
 * @param p_rhs A CAN bus.
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const can::settings& p_lhs,
                                        const can::settings& p_rhs)
{
  return equals(p_lhs.baud_rate, p_rhs.baud_rate) &&
         p_lhs.propagation_delay == p_rhs.propagation_delay &&
         p_lhs.phase_segment1 == p_rhs.phase_segment1 &&
         p_lhs.phase_segment2 == p_rhs.phase_segment2 &&
         p_lhs.synchronization_jump_width == p_rhs.synchronization_jump_width;
}

[[nodiscard]] constexpr std::uint16_t bit_width(const can::settings& p_settings)
{
  // The sum of 4x 8-bit numbers can never exceed uint16_t and thus this
  // operation is always safe.
  return static_cast<std::uint16_t>(
    p_settings.sync_segment + p_settings.propagation_delay +
    p_settings.phase_segment1 + p_settings.phase_segment2);
}

/**
 * @ingroup CAN_Utilities
 * @brief Validate configuration settings against an operating frequency
 *
 * The settings and frequency must follow the following rules:
 *
 * 1. propagation_delay, phase_segment1, phase_segment2 and
 *    synchronization_jump_width must be nonzero.
 * 2. synchronization_jump_width must be the lesser between phase_segment1
 *    and phase_segment2.
 * 3. The total bit width must be equal to or greater than 8 Tq/bit; the sum
 *    of sync_segment, propagation_delay, phase_segment1 and phase_segment2.
 * 4. The CAN device's operating frequency must be at least 8 times the baud
 *    rate to give the minimum.
 * 5. The ratio between the CAN device's operating frequency and the bit
 *    width must be close enough to an integer to produce a usable baud
 *    rate prescaler.
 *
 * @param p_settings - settings object to check
 * @param p_operating_frequency - CAN device operating frequency
 * @return std::optional<std::uint32_t> - baud rate prescaler
 */
[[nodiscard]] constexpr std::optional<std::uint32_t> is_valid(
  const can::settings& p_settings,
  hertz p_operating_frequency)
{
  // 1. propagation_delay, phase_segment1, phase_segment2 and
  //    synchronization_jump_width must be nonzero.
  if (p_settings.propagation_delay == 0 || p_settings.phase_segment1 == 0 ||
      p_settings.phase_segment2 == 0 ||
      p_settings.synchronization_jump_width == 0) {
    return std::nullopt;
  }

  // 2. synchronization_jump_width must be the lesser between phase_segment1
  //    and phase_segment2.
  if (p_settings.synchronization_jump_width > 4 ||
      p_settings.synchronization_jump_width > p_settings.phase_segment1 ||
      p_settings.synchronization_jump_width > p_settings.phase_segment2) {
    return std::nullopt;
  }

  const std::uint16_t bit_width_v = bit_width(p_settings);

  // 3. The total bit width must be equal to or greater than 8 Tq/bit; the
  //    sum of sync_segment, propagation_delay, phase_segment1 and
  //    phase_segment2.
  if (bit_width_v < 8) {
    return std::nullopt;
  }

  // 4. The CAN device's operating frequency must be at least 8 times the
  //    bit rate to give the minimum.
  // 5. The ratio between the CAN device's operating frequency and the bit
  //    width must be close enough to an integer to produce a usable BRP.
  const float bit_width_float = bit_width_v;
  const float scaled_baud = p_settings.baud_rate * bit_width_float;
  const float baud_rate_prescaler = p_operating_frequency / scaled_baud;
  const auto final_prescaler = std::lround(baud_rate_prescaler);

  if (final_prescaler == 0) {
    return std::nullopt;
  }

  return final_prescaler;
}

/**
 * @ingroup CAN_Utilities
 * @brief Compares two CAN message states.
 *
 * @param p_lhs A CAN message.
 * @param p_rhs A CAN message.
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const can::message_t& p_lhs,
                                        const can::message_t& p_rhs)
{
  bool payload_equal = p_lhs.payload == p_rhs.payload;
  return payload_equal && p_lhs.id == p_rhs.id &&
         p_lhs.length == p_rhs.length &&
         p_lhs.is_remote_request == p_rhs.is_remote_request;
}
}  // namespace hal