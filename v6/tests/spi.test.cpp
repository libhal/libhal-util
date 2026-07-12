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

#include <algorithm>
#include <array>
#include <coroutine>

#include <boost/ut.hpp>

import hal;
import hal.util;
import test_util;

namespace {

constexpr hal::byte failure_filler{ 0x33 };
constexpr hal::byte filler_byte{ 'C' };

class test_spi : public hal::spi_channel
{
public:
  ~test_spi() override = default;

  mem::scatter_span<hal::byte const> m_out{};
  mem::scatter_span<hal::byte> m_in{};
  hal::byte m_filler = hal::byte{ 0xFF };

private:
  async::future<void> driver_configure(async::context&,
                                       settings const&) override
  {
    return {};
  }

  async::future<hal::hertz> driver_clock_rate(async::context&) override
  {
    return hal::hertz{};
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

    m_out = p_data_out;
    m_in = p_data_in;
    m_filler = p_filler;

    for (auto chunk : p_data_in) {
      for (auto& elem : chunk) {
        elem = p_filler;  // we re-used filler byte to fill the input param even
                          // though the filler byte is meant for the output line
                          // when performing a read operation.
      }
    }

    if (p_filler == failure_filler) {
      throw hal::io_error(this);
    }

    return {};
  }
};

async::inplace_context<1024> ctx;

constexpr auto empty_scatter_span = mem::scatter_span<hal::byte>{};
constexpr auto empty_const_scatter_span = mem::scatter_span<hal::byte const>{};

void write_test()
{
  using namespace boost::ut;

  "[success] write"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 4> buffer1{};
    std::array<hal::byte, 8> buffer2{};
    std::array<hal::byte, 2> buffer3{};

    mem::scatter_array<hal::byte const, 3> payload{ buffer1, buffer2, buffer3 };
    // Exercise
    auto future = hal::write(ctx, spi, payload);
    finish(future);

    // Verify
    expect(that %
             mem::scatter_span<hal::byte const>{ buffer1, buffer2, buffer3 } ==
           spi.m_out);
    expect(that % empty_scatter_span == spi.m_in);
  };
}

void read_test()
{
  using namespace boost::ut;

  "[success] read"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 4> expected_buffer;
    mem::scatter_array<hal::byte, 1> payload{ expected_buffer };

    // Exercise
    auto future = hal::read(ctx, spi, payload, filler_byte);
    finish(future);

    // Verify
    expect(filler_byte == spi.m_filler);
    expect(that % payload == spi.m_in);
    expect(that % empty_const_scatter_span == spi.m_out);
  };

  "[failure] read"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 4> expected_buffer;
    mem::scatter_array<hal::byte, 1> payload{ expected_buffer };

    // Exercise
    expect(throws<hal::io_error>([&spi, &payload]() {
      auto future = hal::read(ctx, spi, payload, failure_filler);
      finish(future);
    }))
      << "Exception not thrown!";

    // Verify
    expect(failure_filler == spi.m_filler);
    expect(that % payload == spi.m_in);
    expect(that % empty_const_scatter_span == spi.m_out);
  };

  "[success] read<Length>"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 5> expected_buffer;
    expected_buffer.fill(filler_byte);

    // Exercise
    auto future = hal::read<expected_buffer.size()>(ctx, spi, filler_byte);
    finish(future);
    auto actual_buffer = future.value();

    // Verify
    expect(filler_byte == spi.m_filler);
    expect(expected_buffer == actual_buffer);
    expect(that % empty_const_scatter_span == spi.m_out);
  };
}

}  // namespace

int main()
{
  write_test();
  read_test();
}
