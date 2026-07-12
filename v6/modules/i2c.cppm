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

export module hal.util:i2c;

import hal;
import scatter_span;

import :enumeration;

/**
 * @defgroup I2CUtils I2C Utils
 */
namespace hal::inline v6 {
/**
 * @ingroup I2CUtils
 * @brief write data to a target device on the i2c bus
 *
 * Shorthand for writing i2c.transaction(...) for write only operations.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 */
export async::future<void> write(async::context& p_ctx,
                                 hal::i2c& p_i2c,
                                 hal::byte p_address,
                                 mem::scatter_span<hal::byte const> p_data_out)
{
  co_await p_i2c.transaction(p_ctx, p_address, p_data_out, {});
}

/**
 * @ingroup I2CUtils
 * @brief read bytes from target device on i2c bus
 *
 * Shorthand for writing i2c.transaction(...) for read only operations.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_in - buffer to read bytes into from target device
 */
export async::future<void> read(async::context& p_ctx,
                                hal::i2c& p_i2c,
                                hal::byte p_address,
                                mem::scatter_span<hal::byte> p_data_in)
{
  co_await p_i2c.transaction(p_ctx, p_address, {}, p_data_in);
}

/**
 * @ingroup I2CUtils
 * @brief return array of read bytes from target device on i2c bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @return std::array<hal::byte, bytes_to_read> - array of bytes from target
 * device
 */
export template<size_t bytes_to_read>
[[nodiscard]] async::future<std::array<hal::byte, bytes_to_read>>
read(async::context& p_ctx, hal::i2c& p_i2c, hal::byte p_address)
{
  std::array<hal::byte, bytes_to_read> buffer;
  co_await read(p_ctx, p_i2c, p_address, { buffer });
  co_return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief write and then read bytes from target device on i2c bus
 *
 * This API simply calls transaction. This API is here for consistency across
 * the other other communication protocols such as SPI and serial.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_data_in - buffer to read bytes into from target device
 */
export async::future<void> write_then_read(
  async::context& p_ctx,
  hal::i2c& p_i2c,
  hal::byte p_address,
  mem::scatter_span<hal::byte const> p_data_out,
  mem::scatter_span<hal::byte> p_data_in)
{
  co_await p_i2c.transaction(p_ctx, p_address, p_data_out, p_data_in);
}

/**
 * @ingroup I2CUtils
 * @brief write and then return an array of read bytes from target device on i2c
 * bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read after write
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @return std::array<hal::byte, bytes_to_read> -
 */
export template<size_t bytes_to_read>
[[nodiscard]] async::future<std::array<hal::byte, bytes_to_read>>
write_then_read(async::context& p_ctx,
                hal::i2c& p_i2c,
                hal::byte p_address,
                mem::scatter_span<hal::byte const> p_data_out)
{
  std::array<hal::byte, bytes_to_read> buffer;
  co_await write_then_read(p_ctx, p_i2c, p_address, p_data_out, { buffer });
  co_return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief probe the i2c bus to see if a device exists
 *
 * NOTE: that this utilizes the fact that i2c drivers throw
 * hal::no_such_device when a transaction is performed and the device's
 * address is used on the bus and the device does not respond with an
 * acknowledge.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_i2c - i2c driver for the i2c bus that the device may exist on
 * @param p_address - device to check
 * @return true - if the device appears on the bus
 * @return false - if the devices does not appear on the i2c bus
 */
export [[nodiscard]] async::future<bool> probe(async::context& p_ctx,
                                               hal::i2c& p_i2c,
                                               hal::byte p_address)
{
  // p_data_in: empty placeholder for transaction's data_in
  std::array<hal::byte, 1> data_in{};
  bool device_acknowledged = true;

  try {
    co_await p_i2c.transaction(p_ctx, p_address, {}, { data_in });
  } catch (hal::no_such_device const&) {
    device_acknowledged = false;
  }

  co_return device_acknowledged;
}

/**
 * @ingroup I2CUtils
 * @brief Set of I2C transaction operations
 *
 */
export enum class i2c_operation {
  /// Denotes an i2c write operation
  write = 0,
  /// Denotes an i2c read operation
  read = 1,
};

/**
 * @ingroup I2CUtils
 * @brief Convert 7-bit i2c address to an 8-bit address
 *
 * @param p_address 7-bit i2c address
 * @param p_operation write or read operation
 * @return hal::byte - 8-bit i2c address
 */
export [[nodiscard]] inline hal::byte to_8_bit_address(
  hal::byte p_address,
  i2c_operation p_operation) noexcept
{
  hal::byte v8bit_address = static_cast<hal::byte>(p_address << 1);
  v8bit_address |= hal::value(p_operation);
  return v8bit_address;
}
}  // namespace hal::inline v6
