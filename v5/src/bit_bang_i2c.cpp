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
#include <libhal-util/bit_bang_i2c.hpp>
#include <libhal-util/i2c.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/error.hpp>
#include <libhal/units.hpp>

namespace hal {

namespace {
/**
 * @brief This function is a high speed version of the hal::delay function
 * which operates on ticks
 *
 * @param ticks The amount of ticks this function will delay for
 */
void high_speed_delay(steady_clock* p_steady_clock, uint64_t ticks)
{
  auto const start_time_high = p_steady_clock->uptime();
  uint64_t uptime = 0;

  auto const ticks_until_timeout_high = ticks + start_time_high;

  while (uptime < ticks_until_timeout_high) {
    uptime = p_steady_clock->uptime();
    continue;
  }
}
}  // namespace

// Public
bit_bang_i2c::bit_bang_i2c(pins const& p_pins,
                           steady_clock& p_clock,
                           float const p_duty_cycle,
                           hal::i2c::settings const& p_settings)
  : m_scl(p_pins.scl)
  , m_sda(p_pins.sda)
  , m_clock(&p_clock)
  , m_duty_cycle(p_duty_cycle)
{
  bool valid_duty_cycle = 0.3f <= p_duty_cycle && p_duty_cycle <= 0.7f;
  if (not valid_duty_cycle) {
    hal::safe_throw(hal::operation_not_supported(this));
  }

  m_scl->configure(
    { .resistor = hal::pin_resistor::pull_up, .open_drain = true });
  m_sda->configure(
    { .resistor = hal::pin_resistor::pull_up, .open_drain = true });

  bit_bang_i2c::driver_configure(p_settings);

  std::ignore = hal::probe(*this, 0x00);
  std::ignore = hal::probe(*this, 0x00);
}

/*
  It was decided that no calibration should be done to the calculation for ticks
  in the configure function. In this context, calibration refers to the addition
  of ticks to the high and low clock time, which are derived from the level
  function of the output_pin and the uptime function of the steady_clock. This
  decision was made because it would introduce two critical sections in the code
  that the end user would have to deal with. Additionally, it would only improve
  the accuracy by about 0.1 to 0.01 Hz per clock cycle. This marginal
  improvement in accuracy didn't outweigh the potential drawbacks it would
  introduce to the system. See libhal-soft/demos/saleae_captures for the
  comparisons.
*/

void bit_bang_i2c::driver_configure(settings const& p_settings)
{
  using namespace std::chrono_literals;

  if (p_settings.clock_rate > m_clock->frequency()) {
    hal::safe_throw(hal::operation_not_supported(this));
  }

  using period = std::chrono::nanoseconds::period;

  // Calculate period in nanosecond
  auto period_ns = hal::wavelength<period>(p_settings.clock_rate);
  auto scl_high_time = period_ns * m_duty_cycle;
  auto scl_low_time = period_ns - scl_high_time;

  // Calculate ticks for high and low
  auto const frequency = m_clock->frequency();
  auto const tick_period = hal::wavelength<period>(frequency);

  // calculation for ticks
  m_scl_high_ticks = static_cast<uint64_t>(scl_high_time / tick_period);
  m_scl_low_ticks = static_cast<uint64_t>(scl_low_time / tick_period);
}

void bit_bang_i2c::driver_transaction(
  hal::byte p_address,
  std::span<hal::byte const> p_data_out,
  std::span<hal::byte> p_data_in,
  function_ref<hal::timeout_function> p_timeout)
{
  hal::byte address_to_write;

  // Checks if driver should begin a write operation
  if (not p_data_out.empty()) {
    send_start_condition();
    address_to_write =
      hal::to_8_bit_address(p_address, hal::i2c_operation::write);

    write_address(address_to_write, p_timeout);

    write(p_data_out, p_timeout);
  }

  // Checks if driver should begin a read operation
  if (not p_data_in.empty()) {
    send_start_condition();

    address_to_write =
      hal::to_8_bit_address(p_address, hal::i2c_operation::read);

    write_address(address_to_write, p_timeout);

    read(p_data_in, p_timeout);
  }

  send_stop_condition();
}

void bit_bang_i2c::send_start_condition()
{
  // The start condition requires both the sda and scl lines to be pulled high
  // before sending, so we do that here.
  m_sda->level(true);
  m_scl->level(true);
  high_speed_delay(m_clock, m_scl_high_ticks);
  m_sda->level(false);
  high_speed_delay(m_clock, m_scl_high_ticks);
  m_scl->level(false);
  high_speed_delay(m_clock, m_scl_high_ticks);
}

void bit_bang_i2c::send_stop_condition()
{
  high_speed_delay(m_clock, m_scl_high_ticks);
  m_sda->level(false);
  m_scl->level(true);
  high_speed_delay(m_clock, m_scl_high_ticks);
  m_sda->level(true);
  high_speed_delay(m_clock, m_scl_high_ticks);
}

void bit_bang_i2c::write_address(hal::byte p_address,
                                 function_ref<hal::timeout_function> p_timeout)
{
  // Write the address
  auto acknowledged = write_byte(p_address, p_timeout);

  if (!acknowledged) {
    send_stop_condition();
    hal::safe_throw(hal::no_such_device((p_address >> 1), this));
  }
}

void bit_bang_i2c::write(std::span<hal::byte const> p_data_out,
                         function_ref<hal::timeout_function> p_timeout)
{
  bool acknowledged;
  for (hal::byte const& data : p_data_out) {
    acknowledged = write_byte(data, p_timeout);

    if (!acknowledged) {
      send_stop_condition();
    }
  }
}

bool bit_bang_i2c::write_byte(hal::byte p_byte_to_write,
                              function_ref<hal::timeout_function> p_timeout)
{
  hal::byte bit_to_write = 0;
  for (int32_t i = 7; i >= 0; i--) {
    bit_to_write = static_cast<hal::byte>((p_byte_to_write >> i) & 0x1);
    write_bit(bit_to_write, p_timeout);
  }

  // Look for the ack
  auto ack_bit = read_bit();
  // If ack bit is 0, then it was acknowledged (true)
  return ack_bit == 0;
}

/*
   For writing a bit you want to make set the data line first, then toggle the
   level of the clock then check if the level was indeed toggled or if the
   peripheral is stretching the clock. After this is done, you are able to set
   the clock back low.
*/
void bit_bang_i2c::write_bit(hal::byte p_bit_to_write,
                             function_ref<hal::timeout_function> p_timeout)
{
  m_sda->level(static_cast<bool>(p_bit_to_write));
  high_speed_delay(m_clock, m_scl_high_ticks);
  m_scl->level(true);
  high_speed_delay(m_clock, m_scl_low_ticks);
  // If scl is still low after we set it high, then the peripheral is clock
  // stretching
  while (m_scl->level() == 0) {
    p_timeout();
  }
  m_scl->level(false);
}

void bit_bang_i2c::read(std::span<hal::byte> p_data_in,
                        function_ref<hal::timeout_function> p_timeout)
{
  uint32_t size_of_span = p_data_in.size(), i = 0;
  for (hal::byte& data : p_data_in) {
    data = read_byte();
    i++;

    if (i < size_of_span) {
      // If the iterator isn't done, then we ack whatever data we read
      write_bit(0, p_timeout);

    } else {
      // When the data is done being read in, then send a NACK to tell the
      // slave to stop reading
      write_bit(1, p_timeout);
    }
  }
}

hal::byte bit_bang_i2c::read_byte()
{
  constexpr auto byte_length = 8;
  hal::byte read_byte = 0;
  for (uint32_t i = 1; i <= byte_length; i++) {
    read_byte |= (read_bit() << (byte_length - i));
  }
  return read_byte;
}

hal::byte bit_bang_i2c::read_bit()
{
  m_sda->level(true);

  high_speed_delay(m_clock, m_scl_high_ticks);
  m_scl->level(true);

  // Wait for the scl line to settle
  high_speed_delay(m_clock, m_scl_high_ticks);
  auto const bit_read = static_cast<hal::byte>(m_sda->level());

  // Pull clock line low to be ready for the next bit
  m_scl->level(false);

  return bit_read;
}
}  // namespace hal
