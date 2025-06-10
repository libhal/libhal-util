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

#include <cstdlib>
#include <optional>

#include <libhal/can.hpp>

/**
 * @defgroup CAN_Utilities CAN Utilities
 *
 */

namespace hal {
/**
 * @brief Generic settings for a can peripheral
 * @ingroup CAN_Utilities
 *
 * CAN Bit Quanta Timing Diagram of:
 *
 *                               | <--- sjw ---> |
 *         ____    ______    __________    __________
 *      _/ SYNC \/  PROP  \/   PHASE1   \/   PHASE2   \_
 *       \______/\________/\____________/\____________/
 *                                       ^ Sample point
 */
struct can_bus_divider_t
{
  /**
   * @brief Bus clock rate in hertz
   *
   */
  std::uint8_t clock_divider;

  /**
   * @brief Sync Segment (always 1qt)
   *
   * Initial sync transition, the start of a CAN bit
   */
  static constexpr std::uint8_t sync_segment = 1;

  /**
   * @brief Propagation Delay (1qt ... 8qt)
   *
   * Propagation time It is used to compensate for signal delays across the
   * network.
   */
  std::uint8_t propagation_delay;

  /**
   * @brief Length of Phase Segment 1 (1qt ... 8qt)
   *
   * Determines the bit rate, phase segment 1 acts as a buffer that can be
   * lengthened to resynchronize with the bit stream via the
   * synchronization_jump_width. Includes propagation delay
   */
  std::uint8_t phase_segment1;

  /**
   * @brief Length of Phase Segment 2 (1qt ... 8qt)
   *
   * Determines the bit rate and is like phase segment 1 and occurs after the
   * sampling point. Phase segment 2 can be shortened to resynchronize with
   * the bit stream via the synchronization_jump_width.
   */
  std::uint8_t phase_segment2;

  /**
   * @brief Synchronization jump width (1qt ... 4qt)
   *
   * This is the maximum time by which the bit sampling period may be
   * lengthened or shortened during each cycle to adjust for oscillator
   * mismatch between nodes.
   *
   * This value must be smaller than phase_segment1 and phase_segment2
   */
  std::uint8_t synchronization_jump_width;

  /**
   * @brief The total tq of the structure
   *
   */
  std::uint8_t total_tq;
};

/**
 * @ingroup CAN_Utilities
 * @brief Compares two CAN bus settings.
 *
 * @param p_lhs CAN bus settings
 * @param p_rhs A CAN bus setting to compare against another
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(can_bus_divider_t const& p_lhs,
                                        can_bus_divider_t const& p_rhs)
{
  return p_lhs.clock_divider == p_rhs.clock_divider &&
         p_lhs.propagation_delay == p_rhs.propagation_delay &&
         p_lhs.phase_segment1 == p_rhs.phase_segment1 &&
         p_lhs.phase_segment2 == p_rhs.phase_segment2 &&
         p_lhs.synchronization_jump_width == p_rhs.synchronization_jump_width;
}

[[nodiscard]] constexpr std::uint16_t bit_width(
  can_bus_divider_t const& p_settings)
{
  // The sum of 4x 8-bit numbers can never exceed uint16_t and thus this
  // operation is always safe.
  return static_cast<std::uint16_t>(
    p_settings.sync_segment + p_settings.propagation_delay +
    p_settings.phase_segment1 + p_settings.phase_segment2);
}

/**
 * @brief Calculates the can bus divider values
 * @ingroup CAN_Utilities
 *
 * Preferred method of calculating the bus divider values for a can bus
 * peripheral or device. The algorithm checks each possible time quanta (tq)
 * width from 25tq to 8tq. The algorithm always checks starting with the
 * greatest time quanta in order to achieve the longest bit width and sync jump
 * value. The aim is to provide the most flexibility in the sync jump value
 * which should help in most topologies.
 *
 * @param p_operating_frequency - frequency of the input clock to the can bus
 * bit timing module.
 * @param p_target_baud_rate - the baud (bit) rate to set the can bus to.
 * @return std::optional<can_bus_divider_t> - the resulting dividers for the can
 * bus peripheral. Returns std::nullopt if the target baud rate is not
 * achievable with the provided operating frequency.
 */
[[nodiscard]] inline std::optional<can_bus_divider_t> calculate_can_bus_divider(
  hertz p_operating_frequency,
  hertz p_target_baud_rate)
{
  can_bus_divider_t timing;

  // Set phase segments and propagation delay to balanced values
  timing.propagation_delay = 1;

  // use a value of zero in total tq to know if no tq and divider combo worked
  timing.total_tq = 0;

  if (p_operating_frequency < 0.0f || p_target_baud_rate < 0.0f ||
      p_operating_frequency <= p_target_baud_rate) {
    return std::nullopt;
  }

  std::int32_t operating_frequency = p_operating_frequency;
  std::int32_t desired_baud_rate = p_target_baud_rate;

  using inner_div_t =
    decltype(std::div(operating_frequency, desired_baud_rate));
  inner_div_t division{};

  for (std::uint32_t total_tq = 25; total_tq >= 8; total_tq--) {
    division = std::div(operating_frequency, (desired_baud_rate * total_tq));

    if (division.rem == 0) {
      timing.total_tq = total_tq;
      break;
    }
  }

  if (timing.total_tq == 0) {
    return std::nullopt;
  }

  timing.clock_divider = division.quot;
  timing.phase_segment1 = (timing.total_tq - timing.sync_segment) / 2U;
  timing.phase_segment2 = timing.total_tq - timing.sync_segment -
                          timing.phase_segment1 - timing.propagation_delay;

  // Adjust synchronization jump width (sjw) to a safe value
  timing.synchronization_jump_width =
    std::min<std::uint8_t>(timing.phase_segment1, 4U);

  return timing;
}

/**
 * @brief A hal::can_transceiver wrapper class that makes finding message via ID
 * easier with a find API
 *
 * If your driver plans to use this wrapper, rather than storing both the
 * pointer to the hal::can_transceiver and this object, simply construct this
 * object with the hal::can_transceiver object's address and use this object's
 * transceiver() API to get access to the underlying transceiver implementation.
 * This will ensure that the driver doesn't store the implementation's address
 * twice.
 *
 */
class can_message_finder
{
public:
  /**
   * @brief Construct a new can reader object
   *
   * @param p_transceiver - the transceiver to read messages from
   * @param p_id - the message ID to search for
   */
  can_message_finder(hal::can_transceiver& p_transceiver, hal::u32 p_id)
    : m_transceiver(&p_transceiver)
    , m_id(p_id)
  {
  }

  /**
   * @brief Find a message within the receive buffer matching the input message
   * ID.
   *
   * This API performs a double copy of the can message in order to confirm that
   * the message did not get modified by the driver during the copy.
   *
   * @return std::optional<hal::can_message> - a copy of the can message within
   * the buffer. The return is set to std::nullopt if the message could not be
   * found OR was modified during the copy.
   */
  [[nodiscard]] std::optional<hal::can_message> find()
  {
    auto const buffer = m_transceiver->receive_buffer();
    auto cursor = m_transceiver->receive_cursor();
    hal::can_message message = {};

    // Run through the circular buffer until the cursor is reached or until an
    // ID match is found.
    while (m_receive_cursor != cursor) {
      if (m_id == buffer[m_receive_cursor].id) {
        message = buffer[m_receive_cursor];
        // Can message buffers are typically updated via interrupt which can
        // happen at any time. If the cursor has moved past the received message
        // and changed its bits mid way To ensure that an interrupt has not
        // occurred and modified the bits of the buffer message, a second copy
        // is created in order to perform a stability check.
        hal::can_message const copy = buffer[m_receive_cursor];
        bool const message_is_stable = copy == message;
        bool const copy_id_matches = copy.id == m_id;
        if (message_is_stable and copy_id_matches) {
          // Perform circular increment of the m_receive_cursor
          m_receive_cursor = (m_receive_cursor + 1) % buffer.size();
          return message;
        }
      }
      // Acquire latest cursor
      cursor = m_transceiver->receive_cursor();
      // Perform circular increment of the m_receive_cursor
      m_receive_cursor = (m_receive_cursor + 1) % buffer.size();
    }

    return std::nullopt;
  }

  /**
   * @brief Returns the underlying can hal::can_transceiver allowing
   *
   * This API makes it possible for drivers and applications using this wrapper
   * to fully access the hal::can_transceiver without needing to store the
   * pointer twice.
   *
   * @return hal::can_transceiver& - the underlying hal::can_transceiver
   */
  [[nodiscard]] inline hal::can_transceiver& transceiver() const
  {
    return *m_transceiver;
  }

  /**
   * @brief Returns the ID to being searched for
   *
   * @return auto - the ID being searched for
   */
  [[nodiscard]] inline auto id() const
  {
    return m_id;
  }

private:
  hal::can_transceiver* m_transceiver;
  hal::u32 m_id;
  std::size_t m_receive_cursor = 0;
};
}  // namespace hal
