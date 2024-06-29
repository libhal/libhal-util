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

#include <libhal-util/streams.hpp>

#include <span>

#include <libhal-util/comparison.hpp>
#include <libhal-util/timeout.hpp>
#include <libhal/error.hpp>
#include <libhal/timeout.hpp>
#include <libhal/units.hpp>

namespace hal {
stream_find::stream_find(std::span<hal::byte const> p_sequence)
  : m_sequence(p_sequence)
{
}

std::span<hal::byte const> operator|(
  std::span<hal::byte const> const& p_input_data,
  stream_find& p_self)
{
  if (p_input_data.empty()) {
    return p_input_data;
  }

  if (p_self.m_search_index == p_self.m_sequence.size()) {
    return p_input_data;  // forward to next call
  }

  for (size_t index = 0; index < p_input_data.size(); index++) {
    // Check if the next byte received matches the sequence
    if (p_self.m_sequence[p_self.m_search_index] == p_input_data[index]) {
      p_self.m_search_index++;
    } else {  // Otherwise set the search index back to the start.
      p_self.m_search_index = 0;
    }

    // Check if the search index is equal to the size of the sequence size
    if (p_self.m_search_index == p_self.m_sequence.size()) {
      return p_input_data.subspan(index);
    }
  }

  return p_input_data.subspan(p_input_data.size());
}

work_state stream_find::state()
{
  if (m_search_index == m_sequence.size()) {
    return work_state::finished;
  }
  return work_state::in_progress;
}

stream_fill::stream_fill(std::span<hal::byte> p_buffer)
  : m_buffer(p_buffer)
{
}

stream_fill::stream_fill(std::span<hal::byte> p_buffer,
                         size_t const& p_fill_amount)
  : m_buffer(p_buffer)
  , m_fill_amount(&p_fill_amount)
{
}

std::span<hal::byte const> operator|(
  std::span<hal::byte const> const& p_input_data,
  stream_fill& p_self)
{
  if (p_input_data.empty() || p_self.m_buffer.empty()) {
    return p_input_data;
  }

  auto min_size = std::min(p_input_data.size(), p_self.m_buffer.size());

  if (p_self.m_fill_amount != nullptr) {
    min_size = std::min(*p_self.m_fill_amount, min_size);
  }

  std::copy_n(p_input_data.begin(), min_size, p_self.m_buffer.begin());
  p_self.m_buffer = p_self.m_buffer.subspan(min_size);

  if (p_self.m_fill_amount != nullptr) {
    p_self.m_fill_amount -= min_size;
  }

  return p_input_data.subspan(min_size);
}

work_state stream_fill::state()
{
  if (m_buffer.empty()) {
    return work_state::finished;
  }
  return work_state::in_progress;
}

stream_fill_upto::stream_fill_upto(std::span<hal::byte const> p_sequence,
                                   std::span<hal::byte> p_buffer)
  : m_sequence(p_sequence)
  , m_buffer(p_buffer)
{
}

std::span<hal::byte const> operator|(
  std::span<hal::byte const> const& p_input_data,
  stream_fill_upto& p_self)
{
  if (p_input_data.empty() ||
      p_self.m_sequence.size() == p_self.m_search_index ||
      p_self.m_buffer.empty()) {
    return p_input_data;
  }

  auto remaining_buffer = p_self.unfilled();
  auto min_size = std::min(p_input_data.size(), remaining_buffer.size());

  for (size_t index = 0; index < min_size; index++) {
    // Check if the search index is equal to the size of the sequence size
    if (p_self.m_search_index == p_self.m_sequence.size()) {
      p_self.m_fill_amount += index;
      return p_input_data.subspan(index);
    }

    // Check if the next byte received matches the sequence
    if (p_self.m_sequence[p_self.m_search_index] == p_input_data[index]) {
      p_self.m_search_index++;
    } else {  // Otherwise set the search index back to the start.
      p_self.m_search_index = 0;
    }

    remaining_buffer[index] = p_input_data[index];
  }

  p_self.m_fill_amount += min_size;
  return p_input_data.subspan(min_size);
}

work_state stream_fill_upto::state()
{
  if (m_buffer.empty() && m_search_index != m_sequence.size()) {
    return work_state::failed;
  }
  if (m_search_index == m_sequence.size()) {
    return work_state::finished;
  }
  return work_state::in_progress;
}

std::span<hal::byte> stream_fill_upto::span()
{
  return m_buffer.subspan(0, m_fill_amount);
}

std::span<hal::byte> stream_fill_upto::unfilled()
{
  return m_buffer.subspan(m_fill_amount);
}

stream_skip::stream_skip(size_t p_skip)
  : m_skip(p_skip)
{
}

std::span<hal::byte const> operator|(
  std::span<hal::byte const> const& p_input_data,
  stream_skip& p_self)
{
  if (p_input_data.empty()) {
    return p_input_data;
  }

  if (p_self.m_skip == 0) {
    return p_input_data;
  }

  auto min = std::min(p_input_data.size(), p_self.m_skip);
  p_self.m_skip -= min;
  return p_input_data.subspan(min);
}

work_state stream_skip::state()
{
  return (m_skip == 0) ? work_state::finished : work_state::in_progress;
}
}  // namespace hal