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

#include <span>

#include <libhal/error.hpp>
#include <libhal/spi.hpp>
#include <libhal/units.hpp>

/**
 * @defgroup SPI SPI
 *
 */

namespace hal {
/**
 * @ingroup SPI
 * @brief Compares two SPI objects via their settings.
 *
 * @param p_lhs A SPI object
 * @param p_rhs A SPI object
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const spi::settings& p_lhs,
                                        const spi::settings& p_rhs)
{
  return p_lhs.clock_idles_high == p_rhs.clock_idles_high &&
         p_lhs.clock_rate == p_rhs.clock_rate &&
         p_lhs.data_valid_on_trailing_edge == p_rhs.data_valid_on_trailing_edge;
}

/**
 * @ingroup SPI
 * @brief Write data to the SPI bus and ignore data sent from peripherals on the
 * bus.
 *
 * @param p_spi - spi driver
 * @param p_data_out - data to be written to the SPI bus
 * @return result<hal::spi::transfer_t> - success or failure
 */
[[nodiscard]] inline result<hal::spi::transfer_t> write(
  spi& p_spi,
  std::span<const hal::byte> p_data_out)
{
  return p_spi.transfer(
    p_data_out, std::span<hal::byte>{}, spi::default_filler);
}

/**
 * @ingroup SPI
 * @brief Read data from the SPI bus.
 *
 * Filler bytes will be placed on the write line.
 *
 * @param p_spi - spi driver
 * @param p_data_in - buffer to receive bytes back from the SPI bus
 * @param p_filler - filler data placed on the bus in place of actual write
 * data.
 * @return result<hal::spi::transfer_t> - success or failure
 */
[[nodiscard]] inline result<hal::spi::transfer_t> read(
  spi& p_spi,
  std::span<hal::byte> p_data_in,
  hal::byte p_filler = spi::default_filler)
{
  return p_spi.transfer(std::span<hal::byte>{}, p_data_in, p_filler);
}
/**
 * @ingroup SPI
 * @brief Read data from the SPI bus and return a std::array of bytes.
 *
 * Filler bytes will be placed on the write line.
 *
 * @tparam BytesToRead - Number of bytes to read
 * @param p_spi - spi driver
 * @param p_filler - filler data placed on the bus in place of actual write
 * data.
 * @return result<std::array<hal::byte, BytesToRead>> - any errors
 * associated with this call
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> read(
  spi& p_spi,
  hal::byte p_filler = spi::default_filler)
{
  std::array<hal::byte, BytesToRead> buffer;
  HAL_CHECK(p_spi.transfer(std::span<hal::byte>{}, buffer, p_filler));
  return buffer;
}

/**
 * @ingroup SPI
 * @brief Write data to the SPI bus and ignore data sent from peripherals on the
 * bus then read data from the SPI and fill the write line with filler bytes.
 *
 * This utility function that fits the use case of many SPI devices where a
 * transaction is not full duplex. In many spi devices, full duplex means that
 * as data is being written to the SPI bus, the peripheral device is sending
 * data back on the read line. In most cases, the protocol is to write data to
 * the bus, and ignore the read line because the peripheral is not writing
 * anything meaningful to that line, then reading from SPI bus and writing
 * nothing meaningful to the write line as the peripheral is ignoring that line.
 *
 * @param p_spi - spi driver
 * @param p_data_out - bytes to write to the bus
 * @param p_data_in - buffer to receive bytes back from the SPI bus
 * @param p_filler - filler data placed on the bus when the read operation
 * begins.
 * @return result<hal::spi::transfer_t> - success or failure
 */
[[nodiscard]] inline result<hal::spi::transfer_t> write_then_read(
  spi& p_spi,
  std::span<const hal::byte> p_data_out,
  std::span<hal::byte> p_data_in,
  hal::byte p_filler = spi::default_filler)
{
  HAL_CHECK(write(p_spi, p_data_out));
  HAL_CHECK(read(p_spi, p_data_in, p_filler));
  return {};
}

/**
 * @ingroup SPI
 * @brief Write data to the SPI bus and ignore data sent from peripherals on the
 * bus then read data from the SPI, fill the write line with filler bytes and
 * return an array of bytes.
 *
 * @tparam BytesToRead - Number of bytes to read from the bus
 * @param p_spi - spi driver
 * @param p_data_out - bytes to write to the bus
 * @param p_filler - filler data placed on the bus when the read operation
 * begins.
 * @return result<std::array<hal::byte, BytesToRead>>
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> write_then_read(
  spi& p_spi,
  std::span<const hal::byte> p_data_out,
  hal::byte p_filler = spi::default_filler)
{
  HAL_CHECK(write(p_spi, p_data_out));
  return read<BytesToRead>(p_spi, p_filler);
}
}  // namespace hal
