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

#include <chrono>

#include <libhal-util/bit.hpp>
#include <libhal-util/bit_bang_spi.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/steady_clock.hpp>
#include <libhal/units.hpp>

namespace hal {
// public
bit_bang_spi::bit_bang_spi(pins const& p_pins,
                           hal::steady_clock& p_clock,
                           settings const& p_settings,
                           delay_mode p_delay_mode)
  : m_sck(p_pins.sck)
  , m_copi(p_pins.copi)
  , m_cipo(p_pins.cipo)
  , m_clock(&p_clock)
  , m_delay_mode(p_delay_mode)
{
  m_sck->configure(
    { .resistor = hal::pin_resistor::none, .open_drain = false });
  m_copi->configure(
    { .resistor = hal::pin_resistor::none, .open_drain = false });
  m_cipo->configure({ .resistor = hal::pin_resistor::pull_up });
  hal::bit_bang_spi::driver_configure(p_settings);
  m_sck->level(m_polarity);
}

// private
void bit_bang_spi::driver_configure(settings const& p_settings)
{
  using period = std::chrono::nanoseconds::period;
  m_cycle_duration = hal::wavelength<period>(p_settings.clock_rate) / 2;
  m_polarity = p_settings.clock_polarity;
  m_phase = p_settings.clock_phase;
}

void bit_bang_spi::driver_transfer(std::span<hal::byte const> p_data_out,
                                   std::span<hal::byte> p_data_in,
                                   hal::byte p_filler)
{
  size_t max_length = std::max(p_data_in.size(), p_data_out.size());
  for (size_t i = 0; i < max_length; i++) {
    hal::byte write_byte = p_filler;
    if (i < p_data_out.size()) {
      write_byte = p_data_out[i];
    }
    hal::byte read_byte = 0x00;
    if (m_delay_mode == delay_mode::with) {
      read_byte = transfer_byte(write_byte);
    } else {
      read_byte = transfer_byte_without_delay(write_byte);
    }
    if (i < p_data_in.size()) {
      p_data_in[i] = read_byte;
    }
  }
  m_sck->level(m_polarity);
  m_copi->level(false);
}

hal::byte bit_bang_spi::transfer_byte(hal::byte p_byte_to_write)
{
  hal::byte read_byte = 0;
  for (int i = 0; i < 8; ++i) {
    bool bit = (p_byte_to_write >> (7 - i)) & 0b1;
    bool read_bit = transfer_bit(bit);
    read_byte = (read_byte << 1) | read_bit;
  }
  return read_byte;
}

hal::byte bit_bang_spi::transfer_byte_without_delay(hal::byte p_byte_to_write)
{
  hal::byte read_byte = 0;
  for (int i = 0; i < 8; ++i) {
    bool bit = (p_byte_to_write >> (7 - i)) & 0b1;
    bool read_bit = transfer_bit_without_delay(bit);
    read_byte = (read_byte << 1) | read_bit;
  }
  return read_byte;
}

bool bit_bang_spi::transfer_bit(bool p_bit_to_write)
{
  m_sck->level(m_polarity);
  hal::delay(*m_clock, m_cycle_duration);
  if (m_phase) {
    // when phase is 1 (true), data is written on the falling edge of the clock
    // and data is read in on the leading edge
    bool read_bit = m_cipo->level();
    m_sck->level(!m_polarity);
    hal::delay(*m_clock, m_cycle_duration);
    m_copi->level(p_bit_to_write);
    return read_bit;
  } else {
    // when phase is 0 (false), data is written on the leading edge of the clock
    // and data is read in on the falling edge
    m_copi->level(p_bit_to_write);
    m_sck->level(!m_polarity);
    hal::delay(*m_clock, m_cycle_duration);
    return m_cipo->level();
  }
}

bool bit_bang_spi::transfer_bit_without_delay(bool p_bit_to_write)
{
  m_sck->level(m_polarity);
  if (m_phase) {
    // when phase is 1 (true), data is written on the falling edge of the clock
    // and data is read in on the leading edge
    bool read_bit = m_cipo->level();
    m_sck->level(!m_polarity);
    m_copi->level(p_bit_to_write);
    return read_bit;
  } else {
    // when phase is 0 (false), data is written on the leading edge of the clock
    // and data is read in on the falling edge
    m_copi->level(p_bit_to_write);
    m_sck->level(!m_polarity);
    return m_cipo->level();
  }
}

}  // namespace hal
