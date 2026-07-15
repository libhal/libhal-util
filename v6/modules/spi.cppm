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

module;

#include <array>
#include <coroutine>
#include <span>

export module hal.util:spi;

import hal;
import scatter_span;

/**
 * @defgroup SPI SPI
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup SPI
 * @brief Write data to the spi bus, ignore data on the peripherals receive line
 *
 * This command is useful for spi operations where data, such as a command,
 * must be sent to a device and the device does not respond with anything or the
 * response is not necessary.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_spi - spi driver
 * @param p_data_out - data to be written to the spi bus
 */
export async::future<void> write(async::context& p_ctx,
                                 spi_channel& p_spi,
                                 mem::scatter_span<hal::byte const> p_data_out)
{
  co_await p_spi.transfer(p_ctx, p_data_out);
}

/**
 * @ingroup SPI
 * @brief Read data from the SPI bus.
 *
 * Filler bytes will be placed on the write/transmit line.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_spi - spi driver
 * @param p_data_in - buffer to receive bytes back from the spi bus
 * @param p_filler - filler data placed on the bus in place of actual data.
 */
export async::future<void> read(
  async::context& p_ctx,
  spi_channel& p_spi,
  mem::scatter_span<hal::byte> p_data_in,
  hal::byte p_filler = spi_channel::default_filler)
{
  co_await p_spi.transfer(p_ctx, {}, p_data_in, p_filler);
}
/**
 * @ingroup SPI
 * @brief Read data from the SPI bus and return a std::array of bytes.
 *
 * Filler bytes will be placed on the write line.
 *
 * @tparam bytes_to_read - Number of bytes to read
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_spi - spi driver
 * @param p_filler - filler data placed on the bus in place of actual write
 * data.
 * @return std::array<hal::byte, bytes_to_read> - array containing bytes read
 * from the spi bus
 */
export template<size_t bytes_to_read>
[[nodiscard]] async::future<std::array<hal::byte, bytes_to_read>> read(
  async::context& p_ctx,
  spi_channel& p_spi,
  hal::byte p_filler = spi_channel::default_filler)
{
  std::array<hal::byte, bytes_to_read> buffer;
  co_await read(p_ctx, p_spi, { buffer }, p_filler);
  co_return buffer;
}

/**
 * @ingroup SPI
 * @brief Write data to the SPI bus and ignore data sent from peripherals on the
 * bus then read data from the SPI and fill the write line with filler bytes.
 *
 * This utility function that fits the use case of many spi devices where a spi
 * transfer is not full duplex. In many spi devices, full duplex means that
 * as data is being written to the spi bus, the peripheral device is sending
 * data back on the receive line. In most cases, the device's communication
 * protocol is simply:
 *
 *    1. Write data to the bus, ignore the receive line
 *    2. Read from spi bus, filling the write line with filler
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_spi - spi driver
 * @param p_data_out - bytes to write to the bus
 * @param p_data_in - buffer to receive bytes back from the spi bus
 * @param p_filler - filler data placed on the bus when the read operation
 * begins.
 */
export async::future<void> write_then_read(
  async::context& p_ctx,
  spi_channel& p_spi,
  mem::scatter_span<hal::byte const> p_data_out,
  mem::scatter_span<hal::byte> p_data_in,
  hal::byte p_filler = spi_channel::default_filler)
{
  co_await write(p_ctx, p_spi, p_data_out);
  co_await read(p_ctx, p_spi, p_data_in, p_filler);
}

/**
 * @ingroup SPI
 * @brief Write data to the SPI bus and ignore data sent from peripherals on the
 * bus then read data from the SPI, fill the write line with filler bytes and
 * return an array of bytes.
 *
 * See write_then_read() for details about this function.
 *
 * @tparam bytes_to_read - Number of bytes to read from the bus
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_spi - spi driver
 * @param p_data_out - bytes to write to the bus
 * @param p_filler - filler data placed on the bus when the read operation
 * begins.
 * @return std::array<hal::byte, bytes_to_read> - array containing the bytes
 * read from the spi bus.
 */
export template<size_t bytes_to_read>
[[nodiscard]] async::future<std::array<hal::byte, bytes_to_read>>
write_then_read(async::context& p_ctx,
                spi_channel& p_spi,
                mem::scatter_span<hal::byte const> p_data_out,
                hal::byte p_filler = spi_channel::default_filler)
{
  co_await write(p_ctx, p_spi, p_data_out);
  co_return co_await read<bytes_to_read>(p_ctx, p_spi, p_filler);
}
}  // namespace hal::inline v6
