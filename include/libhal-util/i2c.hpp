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

#include <functional>

#include <libhal/error.hpp>
#include <libhal/i2c.hpp>
#include <libhal/units.hpp>

#include "enum.hpp"
#include "math.hpp"

/**
 * @defgroup I2CUtils I2C Utils
 */

namespace hal {
/**
 * @ingroup I2CUtils
 * @brief Compares two I2C bus states.
 *
 * @param p_lhs A I2C bus.
 * @param p_rhs A I2C bus.
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const i2c::settings& p_lhs,
                                        const i2c::settings& p_rhs)
{
  return equals(p_lhs.clock_rate, p_rhs.clock_rate);
}

/**
 * @ingroup I2CUtils
 * @brief write data to a target device on the i2c bus
 *
 * Shorthand for writing i2c.transfer(...) for write only operations
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_timeout - amount of time to execute the transaction
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t> write(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<const hal::byte> p_data_out,
  timeout auto p_timeout)
{
  return p_i2c.transaction(
    p_address, p_data_out, std::span<hal::byte>{}, p_timeout);
}

/**
 * @ingroup I2CUtils
 * @brief write data to a target device on the i2c bus
 *
 * Shorthand for writing i2c.transfer(...) for write only operations, but never
 * times out. Can be used for devices that never perform clock stretching.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t>
write(i2c& p_i2c, hal::byte p_address, std::span<const hal::byte> p_data_out)
{
  return write(p_i2c, p_address, p_data_out, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @brief read bytes from target device on i2c bus
 *
 * Shorthand for writing i2c.transfer(...) for read only operations
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_in - buffer to read bytes into from target device
 * @param p_timeout - amount of time to execute the transaction
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t> read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<hal::byte> p_data_in,
  timeout auto p_timeout)
{
  return p_i2c.transaction(
    p_address, std::span<hal::byte>{}, p_data_in, p_timeout);
}

/**
 * @ingroup I2CUtils
 * @brief read bytes from target device on i2c bus
 *
 * Shorthand for writing i2c.transfer(...) for read only operations, but never
 * times out. Can be used for devices that never perform clock stretching.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_in - buffer to read bytes into from target device
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t>
read(i2c& p_i2c, hal::byte p_address, std::span<hal::byte> p_data_in)
{
  return read(p_i2c, p_address, p_data_in, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @brief return array of read bytes from target device on i2c bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam BytesToRead - number of bytes to read
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_timeout - amount of time to execute the transaction
 * @return result<std::array<hal::byte, BytesToRead>> - array of
 * bytes from target device or an error.
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>>
read(i2c& p_i2c, hal::byte p_address, timeout auto p_timeout)
{
  std::array<hal::byte, BytesToRead> buffer;
  HAL_CHECK(read(p_i2c, p_address, buffer, p_timeout));
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief return array of read bytes from target device on i2c bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 * This operation will never time out and should only be used with devices that
 * never perform clock stretching.
 *
 * @tparam BytesToRead - number of bytes to read
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @return result<std::array<hal::byte, BytesToRead>> - array of
 * bytes from target device or an error.
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> read(
  i2c& p_i2c,
  hal::byte p_address)
{
  return read<BytesToRead>(p_i2c, p_address, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @brief write and then read bytes from target device on i2c bus
 *
 * This API simply calls transaction. This API is here for consistency across
 * the other other communication protocols such as SPI and serial.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_data_in - buffer to read bytes into from target device
 * @param p_timeout - amount of time to execute the transaction
 *
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t> write_then_read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<const hal::byte> p_data_out,
  std::span<hal::byte> p_data_in,
  timeout auto p_timeout = hal::never_timeout())
{
  return p_i2c.transaction(p_address, p_data_out, p_data_in, p_timeout);
}

/**
 * @brief write and then read bytes from target device on i2c bus
 *
 * This API simply calls transaction. This API is here for consistency across
 * the other other communication protocols such as SPI and serial.
 *
 * This operation will never time out and should only be used with devices that
 * never perform clock stretching.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_data_in - buffer to read bytes into from target device
 *
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t> write_then_read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<const hal::byte> p_data_out,
  std::span<hal::byte> p_data_in)
{
  return write_then_read(
    p_i2c, p_address, p_data_out, p_data_in, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @brief write and then return an array of read bytes from target device on i2c
 * bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam BytesToRead - number of bytes to read after write
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_timeout - amount of time to execute the transaction
 * @return result<std::array<hal::byte, BytesToRead>>
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> write_then_read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<const hal::byte> p_data_out,
  timeout auto p_timeout)
{
  std::array<hal::byte, BytesToRead> buffer;
  HAL_CHECK(write_then_read(p_i2c, p_address, p_data_out, buffer, p_timeout));
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief write and then return an array of read bytes from target device on i2c
 * bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam BytesToRead - number of bytes to read after write
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @return result<std::array<hal::byte, BytesToRead>>
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> write_then_read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<const hal::byte> p_data_out)
{
  return write_then_read<BytesToRead>(
    p_i2c, p_address, p_data_out, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @brief probe the i2c bus to see if a device exists
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address to probe for
 * @return hal::result<hal::i2c::transaction_t> - success or failure
 */
[[nodiscard]] inline hal::result<hal::i2c::transaction_t> probe(
  i2c& p_i2c,
  hal::byte p_address)
{
  // p_data_in: empty placeholder for transcation's data_in
  std::array<hal::byte, 1> data_in;

  // p_timeout: no timeout placeholder for transaction's p_timeout
  timeout auto timeout = hal::never_timeout();

  return p_i2c.transaction(p_address, std::span<hal::byte>{}, data_in, timeout);
}

/**
 * @ingroup I2CUtils
 * @brief Set of I2C transaction operations
 *
 */
enum class i2c_operation
{
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
[[nodiscard]] inline hal::byte to_8_bit_address(
  hal::byte p_address,
  i2c_operation p_operation) noexcept
{
  hal::byte v8bit_address = static_cast<hal::byte>(p_address << 1);
  v8bit_address |= hal::value(p_operation);
  return v8bit_address;
}

}  // namespace hal
