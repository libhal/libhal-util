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

#include <cstdio>

#include <array>
#include <coroutine>
#include <span>
#include <string_view>

export module hal.util:serial;

import hal;
import scatter_span;

import :as_bytes;

/**
 * @defgroup Serial Serial
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup Serial
 * @brief Write bytes to a serial port
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - the data to be written out the port
 */
export async::future<void> write(async::context& p_ctx,
                                 hal::serial& p_serial,
                                 mem::scatter_span<hal::byte const> p_data_out)
{
  return p_serial.write(p_ctx, p_data_out);
}

/**
 * @ingroup Serial
 * @brief Write std::string_view of const char to a serial port
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_serial - the serial port that will be written to
 * @param p_data_out - chars to be written out the port
 */
export async::future<void> write(async::context& p_ctx,
                                 hal::serial& p_serial,
                                 std::string_view p_data_out)
{
  return write(
    p_ctx, p_serial, mem::scatter_span<hal::byte const>{ as_bytes(p_data_out) });
}

/**
 * @brief Shared implementation behind every print<buffer_size, ...>()
 * instantiation.
 *
 * Not exported: this exists purely so that print<N, Parameters...>() and
 * print<M, Parameters...>() (same Parameters, different buffer_size) share
 * one function body instead of each generating its own copy of the
 * snprintf-and-write logic. Only the caller-owned buffer and the thin
 * templated wrapper below differ per buffer_size.
 *
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_serial - serial port to write data to
 * @param p_buffer - working buffer the formatted message is written into;
 * its lifetime is owned by the caller and must outlive this call.
 * @param p_format - printf style null terminated format string
 * @param p_parameters - printf arguments
 */
template<typename... Parameters>
async::future<void> print(async::context& p_ctx,
                          hal::serial& p_serial,
                          std::span<char> p_buffer,
                          char const* p_format,
                          Parameters... p_parameters)
{
  int const unterminated_max_string_size =
    static_cast<int>(p_buffer.size()) - 1;

  int length = std::snprintf(
    p_buffer.data(), p_buffer.size(), p_format, p_parameters...);

  if (length > unterminated_max_string_size) {
    // Print out what was able to be written to the buffer
    length = unterminated_max_string_size;
  }

  return write(p_ctx, p_serial, std::string_view(p_buffer.data(), length));
}

/**
 * @ingroup Serial
 * @brief Write formatted string data to serial buffer
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
 * @param p_ctx - async context for coroutine suspension and resumption.
 * @param p_serial - serial port to write data to
 * @param p_format - printf style null terminated format string
 * @param p_parameters - printf arguments
 */
export template<size_t buffer_size, typename... Parameters>
async::future<void> print(async::context& p_ctx,
                          hal::serial& p_serial,
                          char const* p_format,
                          Parameters... p_parameters)
{
  static_assert(buffer_size > 2);

  // buffer must be owned by this coroutine's own frame (not a plain
  // function's stack frame) since the span handed to the non-exported
  // print() above is retained across suspension points all the way down to
  // the underlying serial driver's write().
  std::array<char, buffer_size> buffer{};
  co_await print(
    p_ctx, p_serial, std::span<char>(buffer), p_format, p_parameters...);
}
}  // namespace hal::inline v6
