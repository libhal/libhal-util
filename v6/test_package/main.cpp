// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <array>
#include <print>
#include <string_view>

import hal;
import hal.util;

namespace {
class stub_spi_channel : public hal::spi_channel
{
public:
  mem::scatter_span<hal::byte const> m_last_out{};
  mem::scatter_span<hal::byte> m_last_in{};

private:
  void driver_configure(settings const&) override
  {
  }

  hal::hertz driver_clock_rate() override
  {
    return 1'000'000 * mp_units::si::unit_symbols::Hz;
  }

  void driver_chip_select(bool) override
  {
  }

  void driver_transfer(mem::scatter_span<hal::byte const> p_data_out,
                       mem::scatter_span<hal::byte> p_data_in,
                       hal::byte p_filler) override
  {
    m_last_out = p_data_out;
    m_last_in = p_data_in;
    for (auto chunk : p_data_in) {
      for (auto& element : chunk) {
        element = p_filler;
      }
    }
  }
};

class stub_i2c : public hal::i2c
{
public:
  hal::byte m_last_address{};

private:
  void driver_transaction(hal::byte p_address,
                          mem::scatter_span<hal::byte const>,
                          mem::scatter_span<hal::byte> p_data_in) override
  {
    m_last_address = p_address;
    for (auto chunk : p_data_in) {
      for (auto& element : chunk) {
        element = hal::byte{ 0xA5 };
      }
    }
  }
};
}  // namespace

int main()
{
  using namespace std::string_view_literals;

  auto array = hal::to_array<5>("Hello World!\n"sv);
  std::println("{}", array);

  // SPI demo
  stub_spi_channel spi;
  std::array<hal::byte, 3> const spi_command{ 0x9F, 0x00, 0x00 };
  mem::scatter_array<hal::byte const, 1> spi_payload{ spi_command };

  hal::write(spi, spi_payload);

  auto spi_id_bytes = hal::write_then_read<3>(spi, spi_payload);
  std::println("spi write_then_read<3>: {:#x} {:#x} {:#x}",
               spi_id_bytes[0],
               spi_id_bytes[1],
               spi_id_bytes[2]);

  // I2C demo
  stub_i2c i2c;
  constexpr hal::byte device_address{ 0x50 };

  bool const device_found = hal::probe(i2c, device_address);
  std::println("i2c device found: {}", device_found);

  auto register_bytes = hal::read<2>(i2c, device_address);
  std::println(
    "i2c read<2>: {:#x} {:#x}", register_bytes[0], register_bytes[1]);

  return 0;
}
