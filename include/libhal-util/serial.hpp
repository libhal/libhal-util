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

#include <cstdio>
#include <span>
#include <string_view>

#include <libhal/error.hpp>
#include <libhal/serial.hpp>
#include <libhal/timeout.hpp>
#include <libhal/units.hpp>

#include "as_bytes.hpp"
#include "comparison.hpp"
#include "math.hpp"

/**
 * @defgroup Serial Serial
 *
 */

namespace hal {
/**
 * @ingroup Serial
 * @brief Compares two serial objects via their settings.
 *
 * @param p_lhs A serial object
 * @param p_rhs A serial object
 * @return A boolean if they are the same or not.
 */
[[nodiscard]] constexpr auto operator==(const serial::settings& p_lhs,
                                        const serial::settings& p_rhs)
{
  return equals(p_lhs.baud_rate, p_rhs.baud_rate) &&
         p_lhs.parity == p_rhs.parity && p_lhs.stop == p_rhs.stop;
}
/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 * @return result<serial::write_t> - get the results of the uart port write
 * operation.
 */
[[nodiscard]] inline result<serial::write_t> write_partial(
  serial& p_serial,
  std::span<const hal::byte> p_data_out)
{
  return p_serial.write(p_data_out);
}

/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 * @return status - success or failure
 */
[[nodiscard]] inline status write(serial& p_serial,
                                  std::span<const hal::byte> p_data_out)
{
  auto remaining = p_data_out;

  while (remaining.size() != 0) {
    auto write_length = HAL_CHECK(p_serial.write(remaining)).data.size();
    remaining = remaining.subspan(write_length);
  }

  return success();
}

/**
 * @ingroup Serial
 * @brief Write std::span of const char to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - chars to be written out the port
 * @return status - success or failure
 */
[[nodiscard]] inline status write(serial& p_serial, std::string_view p_data_out)
{
  return write(p_serial, as_bytes(p_data_out));
}

/**
 * @ingroup Serial
 * @brief Read bytes from a serial port
 *
 * @param p_serial - the serial port that will be read from
 * @param p_data_in - buffer to have bytes from the serial port read into
 * @param p_timeout - timeout callable that indicates when to bail out of the
 * read operation.
 * @return result<std::span<hal::byte>> - return an error if
 * a call to serial::read or delay() returns an error from the serial port or
 * a span with the number of bytes read and a pointer to where the read bytes
 * are.
 */
[[nodiscard]] inline result<std::span<hal::byte>>
read(serial& p_serial, std::span<hal::byte> p_data_in, timeout auto p_timeout)
{
  auto remaining = p_data_in;

  while (remaining.size() != 0) {
    auto read_length = HAL_CHECK(p_serial.read(remaining)).data.size();
    remaining = remaining.subspan(read_length);
    HAL_CHECK(p_timeout());
  }

  return p_data_in;
}

/**
 * @ingroup Serial
 * @brief Read bytes from a serial port and return an array.
 *
 * This call eliminates the boiler plate of creating an array and then passing
 * that to the read function.
 *
 * @tparam BytesToRead - the number of bytes to be read from the serial port.
 * @param p_serial - the serial port to be read from
 * @param p_timeout - timeout callable that indicates when to bail out of the
 * read operation.
 * @return result<std::array<hal::byte, BytesToRead>> - return an
 * error if a call to serial::read or delay() returns an error from the
 * serial port or a span with the number of bytes read and a pointer to where
 * the read bytes are.
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> read(
  serial& p_serial,
  timeout auto p_timeout)
{
  std::array<hal::byte, BytesToRead> buffer;
  HAL_CHECK(read(p_serial, buffer, p_timeout));
  return buffer;
}

/**
 * @ingroup Serial
 * @brief Perform a write then read transaction over serial.
 *
 * This is especially useful for devices that use a command and response method
 * of communication.
 *
 * @param p_serial - the serial port to have the transaction occur on
 * @param p_data_out - the data to be written to the port
 * @param p_data_in - a buffer to receive the bytes back from the port
 * @param p_timeout - timeout callable that indicates when to bail out of the
 * read operation.
 * @return status - success or failure
 * or serial::write() returns an error from the serial port or success.
 */
[[nodiscard]] inline result<std::span<hal::byte>> write_then_read(
  serial& p_serial,
  std::span<const hal::byte> p_data_out,
  std::span<hal::byte> p_data_in,
  timeout auto p_timeout)
{
  HAL_CHECK(write_partial(p_serial, p_data_out));
  return read(p_serial, p_data_in, p_timeout);
}

/**
 * @ingroup Serial
 * @brief Perform a write then read transaction over serial.
 *
 * This is especially useful for devices that use a command and response method
 * of communication.
 *
 * @tparam BytesToRead - the number of bytes to read back
 * @param p_serial - the serial port to have the transaction occur on
 * @param p_data_out - the data to be written to the port
 * @param p_timeout - timeout callable that indicates when to bail out of the
 * read operation.
 * @return result<std::array<hal::byte, BytesToRead>> - return an
 * error if a call to serial::read or serial::write() returns an error from the
 * serial port or an array of read bytes.
 */
template<size_t BytesToRead>
[[nodiscard]] result<std::array<hal::byte, BytesToRead>> write_then_read(
  serial& p_serial,
  std::span<const hal::byte> p_data_out,
  timeout auto p_timeout)
{
  std::array<hal::byte, BytesToRead> buffer;
  HAL_CHECK(write_then_read(p_serial, p_data_out, buffer, p_timeout));
  return buffer;
}

/**
 * @ingroup Serial
 * @brief Write data to serial buffer and drop return value
 *
 * Only use this with serial ports with infallible write operations, meaning
 * they will never return an error result.
 *
 * @tparam DataArray - data array type
 * @param p_serial - serial port to write data to
 * @param p_data - data to be sent over the serial port
 */
template<typename DataArray>
void print(serial& p_serial, DataArray&& p_data)
{
  (void)hal::write(p_serial, p_data);
}

/**
 * @ingroup Serial
 * @brief Write formatted string data to serial buffer and drop return value
 *
 * Uses snprintf internally and writes to a local statically allocated an array.
 * This function will never dynamically allocate like how standard std::printf
 * does.
 *
 * This function does NOT include the NULL character when transmitting the data
 * over the serial port.
 *
 * @tparam BufferSize - Size of the buffer to allocate on the stack to store the
 * formatted message.
 * @tparam Parameters - printf arguments
 * @param p_serial - serial port to write data to
 * @param p_format - printf style null terminated format string
 * @param p_parameters - printf arguments
 */
template<size_t BufferSize, typename... Parameters>
void print(serial& p_serial, const char* p_format, Parameters... p_parameters)
{
  static_assert(BufferSize > 2);

  std::array<char, BufferSize> buffer{};
  constexpr int unterminated_max_string_size = static_cast<int>(BufferSize) - 1;

  int length =
    std::snprintf(buffer.data(), buffer.size(), p_format, p_parameters...);

  if (length > unterminated_max_string_size) {
    // Print out what was able to be written to the buffer
    length = unterminated_max_string_size;
  }

  (void)hal::write(p_serial, std::string_view(buffer.data(), length));
}
}  // namespace hal
