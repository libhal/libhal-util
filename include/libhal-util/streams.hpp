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
#include <type_traits>

#include <libhal/error.hpp>
#include <libhal/timeout.hpp>
#include <libhal/units.hpp>

/**
 * @defgroup Streams Streams
 *
 */

namespace hal {
/**
 * @ingroup Streams
 * @brief Discard received bytes until the sequence is found
 *
 */
class stream_find
{
public:
  /**
   * @ingroup Streams
   * @brief Construct a new find object
   *
   * @param p_sequence - sequence to search for. The lifetime of this data
   * pointed to by this span must outlive this object, or not be used when the
   * lifetime of that data is no longer available.
   */
  explicit stream_find(std::span<hal::byte const> p_sequence);

  friend std::span<hal::byte const> operator|(
    std::span<hal::byte const> const& p_input_data,
    stream_find& p_self);

  work_state state();

private:
  std::span<hal::byte const> m_sequence;
  size_t m_search_index = 0;
};

/**
 * @ingroup Streams
 * @brief Non-blocking callable for reading serial data into a buffer
 *
 */
class stream_fill
{
public:
  /**
   * @ingroup Streams
   * @brief Construct a new fill object
   *
   * @param p_buffer - buffer to read data into
   */
  explicit stream_fill(std::span<hal::byte> p_buffer);

  /**
   * @ingroup Streams
   * @brief Construct a new fill object
   *
   * @param p_buffer - buffer to read data into
   * @param p_fill_amount - reference to a size value to limit the fill amount
   * by.
   */
  stream_fill(std::span<hal::byte> p_buffer, size_t const& p_fill_amount);

  friend std::span<hal::byte const> operator|(
    std::span<hal::byte const> const& p_input_data,
    stream_fill& p_self);

  work_state state();

private:
  std::span<hal::byte> m_buffer;
  size_t const* m_fill_amount = nullptr;
};

/**
 * @ingroup Streams
 * @brief Discard received bytes until the sequence is found
 *
 */
class stream_fill_upto
{
public:
  /**
   * @ingroup Streams
   * @brief Construct a new fill upto object
   *
   * @param p_sequence - sequence to search for. The lifetime of this data
   * pointed to by this span must outlive this object, or not be used when the
   * lifetime of that data is no longer available.
   * @param p_buffer - buffer to fill data into
   */
  stream_fill_upto(std::span<hal::byte const> p_sequence,
                   std::span<hal::byte> p_buffer);

  friend std::span<hal::byte const> operator|(
    std::span<hal::byte const> const& p_input_data,
    stream_fill_upto& p_self);

  work_state state();

  std::span<hal::byte> span();
  std::span<hal::byte> unfilled();

private:
  std::span<hal::byte const> m_sequence;
  std::span<hal::byte> m_buffer;
  size_t m_fill_amount = 0;
  size_t m_search_index = 0;
};

/**
 * @ingroup Streams
 * @brief Read bytes from stream and convert to integer
 *
 */
template<std::unsigned_integral T>
class stream_parse
{
public:
  /**
   * @ingroup Streams
   * @brief Construct a new parse object
   */
  explicit stream_parse() = default;

  friend std::span<hal::byte const> operator|(
    std::span<hal::byte const> const& p_input_data,
    stream_parse& p_self)
  {
    if (p_self.m_finished) {
      return p_input_data;
    }

    for (size_t index = 0; index < p_input_data.size(); index++) {
      if (std::isdigit(static_cast<char>(p_input_data[index]))) {
        p_self.m_value *= 10;
        p_self.m_value += p_input_data[index] - hal::byte('0');
        p_self.m_found_digit = true;
      } else if (p_self.m_found_digit) {
        p_self.m_finished = true;
        return p_input_data.subspan(index);
      }
    }

    return p_input_data.last(0);
  }

  work_state state()
  {
    return m_finished ? work_state::finished : work_state::in_progress;
  }

  /**
   * @return T& - return an immutable reference to the value
   */
  T const& value()
  {
    return m_value;
  }

private:
  T m_value = 0;
  bool m_found_digit = false;
  bool m_finished = false;
};

/**
 * @ingroup Streams
 * @brief Skip number of bytes in a byte stream
 *
 */
class stream_skip
{
public:
  /**
   * @ingroup Streams
   * @brief Construct a new skip object
   *
   */
  explicit stream_skip(size_t p_skip);

  friend std::span<hal::byte const> operator|(
    std::span<hal::byte const> const& p_input_data,
    stream_skip& p_self);

  work_state state();

private:
  size_t m_skip;
};
}  // namespace hal
