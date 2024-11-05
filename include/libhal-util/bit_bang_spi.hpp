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
#include <libhal/output_pin.hpp>
#include <libhal/spi.hpp>
#include <libhal/steady_clock.hpp>

namespace hal::soft {
/**
 * @brief A bit bang implementation for spi.
 *
 * This implementation of spi only needs 2 hal::output_pins for sck and copi, a
 * hal::input_pin for cipo, and a steady_clock to work.
 */
class bit_bang_spi : public spi
{
public:
  struct pins
  {
    output_pin* sck;
    output_pin* copi;
    input_pin* cipo;
  };

  /// Adds or removes delays in the read/write cycle
  enum class delay_mode
  {
    /// Calculates the delay time using the clock_rate from the provided
    /// settings when constructing the bit_bang_spi object
    with,
    /// Omits delays between reading/writing to get the fastest speed possible
    without
  };

  /**
   * @brief Construct a new bit bang spi object
   *
   * @param p_pins named structure that contains pointers to the sck, copi, and
   * cipo to be used inside of the driver
   * @param p_steady_clock the steady clock that will be used for timing the sck
   * line.
   * @param p_settings the initial settings of the spi bus
   * @param p_delay_mode adds or removes delays in the read/write cycle
   */
  bit_bang_spi(pins const& p_pins,
               steady_clock& p_steady_clock,
               settings const& p_settings = {},
               delay_mode p_delay_mode = delay_mode::with);

private:
  void driver_configure(settings const& p_settings) override;

  void driver_transfer(std::span<hal::byte const> p_data_out,
                       std::span<hal::byte> p_data_in,
                       hal::byte p_filler) override;

  /**
   * @brief This function will handle writing a single byte to spi copi line
   *
   * @param p_byte_to_write This is the byte that will be written
   */
  hal::byte transfer_byte(hal::byte p_byte_to_write);

  /**
   * @brief This function will handle writing a single byte to spi copi line
   * without using delays between reading and writing
   *
   * @param p_byte_to_write This is the byte that will be written
   */
  hal::byte transfer_byte_without_delay(hal::byte p_byte_to_write);

  /**
   * @brief This function will handle writing a single bit to the spi copi line
   *
   * @param p_bit_to_write This is the bit that will be written
   */
  bool transfer_bit(bool p_bit_to_write);

  /**
   * @brief This function will handle writing a single bit to the spi copi line
   * without using delays between reading and writing
   *
   * @param p_bit_to_write This is the bit that will be written
   */
  bool transfer_bit_without_delay(bool p_bit_to_write);

  /// @brief An output pin which is the spi sck pin
  hal::output_pin* m_sck;

  /// @brief An output pin which is the spi copi pin
  hal::output_pin* m_copi;

  /// @brief An input pin which is the spi cipo pin
  hal::input_pin* m_cipo;

  /// @brief A steady_clock provides a mechanism to delay the clock pulses of
  /// the sck line.
  hal::steady_clock* m_clock;

  /// @brief State of the sck line when inactive
  bool m_polarity;

  /// @brief Phase of the sck line dictates when to read/write bits
  bool m_phase;

  /// @brief Time of a full read/write cycle, used in delay_mode::with
  hal::time_duration m_cycle_duration;

  /// @brief Configuration for using delays or not
  delay_mode m_delay_mode;
};

}  // namespace hal::soft
