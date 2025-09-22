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

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory_resource>
#include <span>
#include <string_view>

#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/timeout.hpp>
#include <libhal/units.hpp>

#include "as_bytes.hpp"

/**
 * @defgroup Serial Serial
 *
 */

namespace hal {
// TODO(#38): Remove on release of libhal 5.0.0
/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 * @return serial::write_t - information about the write_t operation.
 */
[[nodiscard]] inline serial::write_t write_partial(
  serial& p_serial,
  std::span<hal::byte const> p_data_out)
{
  return p_serial.write(p_data_out);
}

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

/**
 * @brief Converts a hal::v5::serial interface to hal::serial interface
 *
 * This adapter allows legacy code using hal::serial to work with new
 * hal::v5::serial implementations. It manages the conversion between
 * the cursor-based circular buffer approach of v5 and the traditional
 * read/write buffer approach of the legacy interface.
 *
 * The converter directly reads from the v5 serial's existing circular buffer,
 * eliminating the need for additional internal buffering.
 *
 * Uses hal::v5::strong_ptr for memory-safe reference counting of the underlying
 * v5 serial implementation.
 */
class serial_v5_to_legacy_converter : public hal::serial
{
public:
  /**
   * @brief Construct converter from v5 serial strong_ptr
   *
   * @param p_v5_serial strong_ptr to the v5 serial implementation to wrap
   */
  explicit serial_v5_to_legacy_converter(
    hal::v5::strong_ptr<hal::v5::serial> const& p_v5_serial)
    : m_v5_serial(p_v5_serial)
    , m_last_cursor(m_v5_serial->receive_cursor())
  {
  }

  /**
   * @brief Get access to the underlying v5 serial implementation
   *
   * @return const reference to the strong_ptr holding the v5 serial
   */
  [[nodiscard]] hal::v5::strong_ptr<hal::v5::serial> v5_serial()
  {
    return m_v5_serial;
  }

private:
  void driver_configure(settings const& p_settings) override
  {
    // Convert legacy settings to v5 settings
    hal::v5::serial::settings v5_settings;
    v5_settings.baud_rate = static_cast<u32>(p_settings.baud_rate);

    // Convert stop bits
    switch (p_settings.stop) {
      case settings::stop_bits::one:
        v5_settings.stop = hal::v5::serial::settings::stop_bits::one;
        break;
      case settings::stop_bits::two:
        v5_settings.stop = hal::v5::serial::settings::stop_bits::two;
        break;
    }

    // Convert parity
    switch (p_settings.parity) {
      case settings::parity::none:
        v5_settings.parity = hal::v5::serial::settings::parity::none;
        break;
      case settings::parity::odd:
        v5_settings.parity = hal::v5::serial::settings::parity::odd;
        break;
      case settings::parity::even:
        v5_settings.parity = hal::v5::serial::settings::parity::even;
        break;
      case settings::parity::forced1:
        v5_settings.parity = hal::v5::serial::settings::parity::forced1;
        break;
      case settings::parity::forced0:
        v5_settings.parity = hal::v5::serial::settings::parity::forced0;
        break;
    }

    m_v5_serial->configure(v5_settings);
  }

  write_t driver_write(std::span<byte const> p_data) override
  {
    // v5 interface doesn't return partial write info, so we assume all data is
    // written
    m_v5_serial->write(p_data);
    return write_t{ .data = p_data };
  }

  read_t driver_read(std::span<byte> p_data) override
  {
    auto const v5_buffer = m_v5_serial->receive_buffer();
    auto const current_cursor = m_v5_serial->receive_cursor();

    // Calculate how many new bytes are available since last read
    auto const buffer_size = v5_buffer.size();
    auto const available_bytes =
      (current_cursor + buffer_size - m_last_cursor) % buffer_size;

    // Copy as much as we can into the user's buffer
    auto const bytes_to_copy = std::min(p_data.size(), available_bytes);

    if (bytes_to_copy > 0) {
      // Handle potential wraparound in the v5 circular buffer
      auto const first_chunk_size =
        std::min(bytes_to_copy, buffer_size - m_last_cursor);

      // Copy the first chunk (from last_cursor to end of buffer or
      // bytes_to_copy)
      std::copy_n(v5_buffer.subspan(m_last_cursor).begin(),
                  first_chunk_size,
                  p_data.begin());

      // If we need more bytes and they wrapped around to the beginning
      if (bytes_to_copy > first_chunk_size) {
        auto remaining = bytes_to_copy - first_chunk_size;
        std::copy_n(v5_buffer.begin(),
                    remaining,
                    p_data.subspan(first_chunk_size).begin());
      }

      // Update our cursor position
      m_last_cursor = (m_last_cursor + bytes_to_copy) % buffer_size;
    }

    // Calculate remaining available bytes after this read
    auto const remaining_available = available_bytes - bytes_to_copy;

    return read_t{ .data = p_data.subspan(0, bytes_to_copy),
                   .available = remaining_available,
                   .capacity = buffer_size };
  }

  void driver_flush() override
  {
    // Reset our cursor to the current position, effectively "flushing" our view
    // of the data. The v5 interface doesn't have flush, so we just reset
    // tracking.
    m_last_cursor = m_v5_serial->receive_cursor();
  }

  hal::v5::strong_ptr<hal::v5::serial> m_v5_serial;
  usize m_last_cursor;
};

/**
 * @brief Convenience function to create a serial converter
 *
 * @param p_allocator Allocator for creating the converter
 * @param p_v5_serial strong_ptr to v5 serial implementation
 * @return strong_ptr to converter instance
 */
[[nodiscard]] inline hal::v5::strong_ptr<serial_v5_to_legacy_converter>
make_serial_converter(std::pmr::polymorphic_allocator<hal::byte> p_allocator,
                      hal::v5::strong_ptr<hal::v5::serial> const& p_v5_serial)
{
  return hal::v5::make_strong_ptr<serial_v5_to_legacy_converter>(p_allocator,
                                                                 p_v5_serial);
}
}  // namespace hal

namespace hal::v5 {
/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * Unlike write_partial, this function will attempt to write bytes to the serial
 * port until the timeout time has expired.
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 */
inline void write(hal::v5::strong_ptr<v5::serial> const& p_serial,
                  std::span<hal::byte const> p_data_out)
{
  p_serial->write(p_data_out);
}

/**
 * @ingroup Serial
 * @brief Write `std::string_view` of const char to a serial port
 *
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - chars to be written out the port
 */
inline void write(hal::v5::strong_ptr<v5::serial> const& p_serial,
                  std::string_view p_data_out)
{
  v5::write(p_serial, as_bytes(p_data_out));
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
void print(hal::v5::strong_ptr<v5::serial> const& p_serial,
           byte_array_t&& p_data)
{
  hal::v5::write(p_serial, p_data);
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
void print(hal::v5::strong_ptr<v5::serial> const& p_serial,
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

  v5::write(p_serial, std::string_view(buffer.data(), length));
}
}  // namespace hal::v5
