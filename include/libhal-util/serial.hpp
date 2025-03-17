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
#include <libhal/zero_copy_serial.hpp>

#include "as_bytes.hpp"

/**
 * @defgroup Serial Serial
 *
 */

namespace hal {
/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * Unlike write_partial, this function will attempt to write bytes to the serial
 * port until the timeout time has expired.
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 * @param p_timeout - timeout callable that throws a std::errc::timed_out
 * exception when the timeout expires.
 * @throws std::errc::timed_out - if p_timeout expires
 */
inline void write(serial& p_serial,
                  std::span<hal::byte const> p_data_out,
                  timeout auto p_timeout)
{
  auto remaining = p_data_out;

  while (remaining.size() != 0) {
    auto write_length = p_serial.write(remaining).data.size();
    remaining = remaining.subspan(write_length);
    p_timeout();
  }
}

/**
 * @ingroup Serial
 * @brief Write std::span of const char to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - chars to be written out the port
 * @param p_timeout - timeout callable that throws a std::errc::timed_out
 * exception when the timeout expires.
 * @throws std::errc::timed_out - if p_timeout expires
 */
inline void write(serial& p_serial,
                  std::string_view p_data_out,
                  timeout auto p_timeout)
{
  write(p_serial, as_bytes(p_data_out), p_timeout);
}

/**
 * @ingroup Serial
 * @brief Read bytes from a serial port
 *
 * @param p_serial - the serial port that will be read from
 * @param p_data_in - buffer to have bytes from the serial port read into. When
 * this function completes, the full contents of p_data_in should contain
 * @param p_timeout - timeout callable that throws a std::errc::timed_out
 * exception when the timeout expires.
 * @throws std::errc::timed_out - if p_timeout expires
 */
inline void read(serial& p_serial,
                 std::span<hal::byte> p_data_in,
                 timeout auto p_timeout)
{
  auto remaining = p_data_in;

  while (remaining.size() != 0) {
    auto read_length = p_serial.read(remaining).data.size();
    remaining = remaining.subspan(read_length);
    p_timeout();
  }
}

/**
 * @ingroup Serial
 * @brief Read bytes from a serial port and return an array.
 *
 * This call eliminates the boiler plate of creating an array and then passing
 * that to the read function.
 *
 * @tparam bytes_to_read - the number of bytes to be read from the serial port.
 * @param p_serial - the serial port to be read from
 * @param p_timeout - timeout callable that throws a std::errc::timed_out
 * exception when the timeout expires.
 * @return std::array<hal::byte, bytes_to_read> - array containing the bytes
 * read from the serial port.
 * @throws std::errc::timed_out - if p_timeout expires
 */
template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> read(serial& p_serial,
                                                        timeout auto p_timeout)
{
  std::array<hal::byte, bytes_to_read> buffer;
  read(p_serial, buffer, p_timeout);
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
 * @param p_timeout - timeout callable that throws a std::errc::timed_out
 * exception when the timeout expires.
 * @throws std::errc::timed_out - if p_timeout expires
 */
inline void write_then_read(serial& p_serial,
                            std::span<hal::byte const> p_data_out,
                            std::span<hal::byte> p_data_in,
                            timeout auto p_timeout)
{
  write(p_serial, p_data_out, p_timeout);
  read(p_serial, p_data_in, p_timeout);
}

/**
 * @ingroup Serial
 * @brief Perform a write then read transaction over serial.
 *
 * This is especially useful for devices that use a command and response method
 * of communication.
 *
 * @tparam bytes_to_read - the number of bytes to read back
 * @param p_serial - the serial port to have the transaction occur on
 * @param p_data_out - the data to be written to the port
 * @param p_timeout - timeout callable that indicates when to bail out of the
 * read operation.
 * @return std::array<hal::byte, bytes_to_read> - array containing the bytes
 * read from the serial port.
 * @throws std::errc::timed_out - if p_timeout expires
 */
template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> write_then_read(
  serial& p_serial,
  std::span<hal::byte const> p_data_out,
  timeout auto p_timeout)
{
  std::array<hal::byte, bytes_to_read> buffer;
  write_then_read(p_serial, p_data_out, buffer, p_timeout);
  return buffer;
}

/**
 * @ingroup Serial
 * @brief Write data to serial buffer and drop return value
 *
 * Only use this with serial ports with infallible write operations, meaning
 * they will never return an error result.
 *
 * @tparam byte_array_t - data array type
 * @param p_serial - serial port to write data to
 * @param p_data - data to be sent over the serial port
 */
template<typename byte_array_t>
void print(serial& p_serial, byte_array_t&& p_data)
{
  hal::write(p_serial, p_data, hal::never_timeout());
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
 * @tparam buffer_size - Size of the buffer to allocate on the stack to store
 * the formatted message.
 * @tparam Parameters - printf arguments
 * @param p_serial - serial port to write data to
 * @param p_format - printf style null terminated format string
 * @param p_parameters - printf arguments
 */
template<size_t buffer_size, typename... Parameters>
void print(serial& p_serial, char const* p_format, Parameters... p_parameters)
{
  static_assert(buffer_size > 2);
  constexpr int unterminated_max_string_size =
    static_cast<int>(buffer_size) - 1;

  std::array<char, buffer_size> buffer{};
  int length =
    std::snprintf(buffer.data(), buffer.size(), p_format, p_parameters...);

  if (length > unterminated_max_string_size) {
    // Print out what was able to be written to the buffer
    length = unterminated_max_string_size;
  }

  hal::write(
    p_serial, std::string_view(buffer.data(), length), hal::never_timeout());
}

// zero copy functions
/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 */
inline void write(zero_copy_serial& p_serial,
                  std::span<hal::byte const> p_data_out)
{
  p_serial.write(p_data_out);
}

/**
 * @ingroup Serial
 * @brief Write std::span of const char to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - chars to be written out the port
 */
inline void write(zero_copy_serial& p_serial, std::string_view p_data_out)
{
  write(p_serial, as_bytes(p_data_out));
}

/**
 * @ingroup Serial
 * @brief Write data to serial buffer and drop return value
 *
 * Only use this with serial ports with infallible write operations, meaning
 * they will never return an error result.
 *
 * @tparam byte_array_t - data array type
 * @param p_serial - serial port to write data to
 * @param p_data - data to be sent over the serial port
 */
template<typename byte_array_t>
void print(zero_copy_serial& p_serial, byte_array_t&& p_data)
{
  hal::write(p_serial, p_data);
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
 * @tparam buffer_size - Size of the buffer to allocate on the stack to store
 * the formatted message.
 * @tparam Parameters - printf arguments
 * @param p_serial - serial port to write data to
 * @param p_format - printf style null terminated format string
 * @param p_parameters - printf arguments
 */
template<size_t buffer_size, typename... Parameters>
void print(zero_copy_serial& p_serial,
           char const* p_format,
           Parameters... p_parameters)
{
  static_assert(buffer_size > 2);
  constexpr int unterminated_max_string_size =
    static_cast<int>(buffer_size) - 1;

  std::array<char, buffer_size> buffer{};
  int length =
    std::snprintf(buffer.data(), buffer.size(), p_format, p_parameters...);

  if (length > unterminated_max_string_size) {
    // Print out what was able to be written to the buffer
    length = unterminated_max_string_size;
  }

  hal::write(p_serial, std::string_view(buffer.data(), length));
}
}  // namespace hal
