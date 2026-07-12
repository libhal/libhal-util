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
#include <coroutine>
#include <print>
#include <string_view>

import hal;
import hal.util;

namespace {
template<typename T>
void finish(async::future<T>& p_future)
{
  while (not p_future.done()) {
    p_future.resume();
  }
}

class stub_spi_channel : public hal::spi_channel
{
public:
  ~stub_spi_channel() override = default;

  mem::scatter_span<hal::byte const> m_last_out{};
  mem::scatter_span<hal::byte> m_last_in{};

private:
  async::future<void> driver_configure(async::context&,
                                       settings const&) override
  {
    return {};
  }

  async::future<hal::hertz> driver_clock_rate(async::context&) override
  {
    co_return 1'000'000 * mp_units::si::unit_symbols::Hz;
  }

  async::future<void> driver_chip_select(async::context&, bool) override
  {
    return {};
  }

  async::future<void> driver_transfer(
    async::context&,
    mem::scatter_span<hal::byte const> p_data_out,
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
    return {};
  }
};

class stub_i2c : public hal::i2c
{
public:
  ~stub_i2c() override = default;

  hal::byte m_last_address{};

private:
  async::future<void> driver_configure(async::context&,
                                       settings const&) override
  {
    return {};
  }

  async::future<void> driver_transaction(
    async::context&,
    hal::byte p_address,
    mem::scatter_span<hal::byte const>,
    mem::scatter_span<hal::byte> p_data_in) override
  {
    m_last_address = p_address;
    for (auto chunk : p_data_in) {
      for (auto& element : chunk) {
        element = hal::byte{ 0xA5 };
      }
    }
    return {};
  }
};
}  // namespace

int main()
{
  using namespace std::string_view_literals;

  auto array = hal::to_array<5>("Hello World!\n"sv);
  std::println("{}", array);

  async::inplace_context<1024> ctx;

  // SPI demo
  stub_spi_channel spi;
  std::array<hal::byte, 3> const spi_command{ 0x9F, 0x00, 0x00 };
  mem::scatter_array<hal::byte const, 1> spi_payload{ spi_command };

  auto spi_write = hal::write(ctx, spi, spi_payload);
  finish(spi_write);

  auto spi_id = hal::write_then_read<3>(ctx, spi, spi_payload);
  finish(spi_id);
  auto spi_id_bytes = spi_id.value();
  std::println("spi write_then_read<3>: {:#x} {:#x} {:#x}",
               spi_id_bytes[0],
               spi_id_bytes[1],
               spi_id_bytes[2]);

  // I2C demo
  stub_i2c i2c;
  constexpr hal::byte device_address{ 0x50 };

  auto probe_future = hal::probe(ctx, i2c, device_address);
  finish(probe_future);
  std::println("i2c device found: {}", probe_future.value());

  auto register_read = hal::read<2>(ctx, i2c, device_address);
  finish(register_read);
  auto register_bytes = register_read.value();
  std::println(
    "i2c read<2>: {:#x} {:#x}", register_bytes[0], register_bytes[1]);

  return 0;
}
