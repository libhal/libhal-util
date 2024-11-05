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

#include <libhal/error.hpp>
#include <libhal/i2c.hpp>
#include <libhal/units.hpp>

#include "enum.hpp"

/**
 * @defgroup I2CUtils I2C Utils
 */
namespace hal {
/**
 * @ingroup I2CUtils
 * @deprecated use APIs that do not use timeouts
 * @brief write data to a target device on the i2c bus with a timeout
 * @deprecated Prefer to use the write API that does not require a timeout.
 *
 * Shorthand for writing i2c.transfer(...) for write only operations
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param auto - [deprecated don't use]
 */
inline void write(i2c& p_i2c,
                  hal::byte p_address,
                  std::span<hal::byte const> p_data_out,
                  timeout auto)
{
  p_i2c.transaction(p_address, p_data_out, std::span<hal::byte>{});
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
 */
inline void write(i2c& p_i2c,
                  hal::byte p_address,
                  std::span<hal::byte const> p_data_out)
{
  write(p_i2c, p_address, p_data_out, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @deprecated use APIs that do not use timeouts
 * @brief read bytes from target device on i2c bus
 *
 * Shorthand for writing i2c.transfer(...) for read only operations
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_in - buffer to read bytes into from target device
 * @param auto - [deprecated don't use]
 */
inline void read(i2c& p_i2c,
                 hal::byte p_address,
                 std::span<hal::byte> p_data_in,
                 timeout auto)
{
  p_i2c.transaction(p_address, std::span<hal::byte>{}, p_data_in);
}

/**
 * @ingroup I2CUtils
 * @deprecated use APIs that do not use timeouts
 * @brief read bytes from target device on i2c bus
 *
 * Shorthand for writing i2c.transfer(...) for read only operations, but never
 * times out. Can be used for devices that never perform clock stretching.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_in - buffer to read bytes into from target device
 */
inline void read(i2c& p_i2c,
                 hal::byte p_address,
                 std::span<hal::byte> p_data_in)
{
  read(p_i2c, p_address, p_data_in, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @deprecated use APIs that do not use timeouts
 * @brief return array of read bytes from target device on i2c bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param auto - [deprecated don't use]
 * @return std::array<hal::byte, bytes_to_read> - array of bytes from target
 * device
 */
template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> read(i2c& p_i2c,
                                                        hal::byte p_address,
                                                        timeout auto)
{
  std::array<hal::byte, bytes_to_read> buffer;
  read(p_i2c, p_address, buffer);
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief return array of read bytes from target device on i2c bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @return std::array<hal::byte, bytes_to_read> - array of bytes from target
 * device
 */
template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> read(i2c& p_i2c,
                                                        hal::byte p_address)
{
  std::array<hal::byte, bytes_to_read> buffer;
  read(p_i2c, p_address, buffer);
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @deprecated use APIs that do not use timeouts
 * @brief write and then read bytes from target device on i2c bus
 *
 * This API simply calls transaction. This API is here for consistency across
 * the other other communication protocols such as SPI and serial.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_data_in - buffer to read bytes into from target device
 * @param auto - amount of time to execute the transaction
 */
inline void write_then_read(i2c& p_i2c,
                            hal::byte p_address,
                            std::span<hal::byte const> p_data_out,
                            std::span<hal::byte> p_data_in,
                            timeout auto)
{
  p_i2c.transaction(p_address, p_data_out, p_data_in);
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
 */
inline void write_then_read(i2c& p_i2c,
                            hal::byte p_address,
                            std::span<hal::byte const> p_data_out,
                            std::span<hal::byte> p_data_in)
{
  return write_then_read(
    p_i2c, p_address, p_data_out, p_data_in, hal::never_timeout());
}

/**
 * @ingroup I2CUtils
 * @deprecated use APIs that do not use timeouts
 * @brief write and then return an array of read bytes from target device on i2c
 * bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read after write
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param auto - [deprecated use the APIs without timeout parameters]
 * @return std::array<hal::byte, bytes_to_read> - array of bytes from target
 * device.
 */
template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> write_then_read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<hal::byte const> p_data_out,
  timeout auto)
{
  std::array<hal::byte, bytes_to_read> buffer;
  write_then_read(p_i2c, p_address, p_data_out, buffer);
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief write and then return an array of read bytes from target device on i2c
 * bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read after write
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @return std::array<hal::byte, bytes_to_read> -
 */
template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> write_then_read(
  i2c& p_i2c,
  hal::byte p_address,
  std::span<hal::byte const> p_data_out)
{
  std::array<hal::byte, bytes_to_read> buffer;
  write_then_read(p_i2c, p_address, p_data_out, buffer);
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief probe the i2c bus to see if a device exists
 *
 * NOTE: that this utilizes the fact that i2c drivers throw
 * std::errc::no_such_device_or_address when a transaction is performed and the
 * device's address is used on the bus and the device does not respond with an
 * acknowledge.
 *
 * @param p_i2c - i2c driver for the i2c bus that the device may exist on
 * @param p_address - device to check
 * @return true - if the device appears on the bus
 * @return false - if the devices does not appear on the i2c bus
 */
[[nodiscard]] inline bool probe(i2c& p_i2c, hal::byte p_address)
{
  // p_data_in: empty placeholder for transcation's data_in
  std::array<hal::byte, 1> data_in{};
  // p_timeout: no timeout placeholder for transaction's p_timeout
  timeout auto timeout = hal::never_timeout();
  bool device_acknowledged = true;

  try {
    p_i2c.transaction(p_address, std::span<hal::byte>{}, data_in, timeout);
  } catch (hal::no_such_device const& p_error) {
    device_acknowledged = false;
  }

  return device_acknowledged;
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
  // TODO(#36): remove noexcept from this function on libhal's 5.0.0 API break
  hal::byte v8bit_address = static_cast<hal::byte>(p_address << 1);
  v8bit_address |= hal::value(p_operation);
  return v8bit_address;
}
}  // namespace hal
