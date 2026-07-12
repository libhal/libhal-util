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

#include <array>
#include <string_view>

#include <boost/ut.hpp>

import hal;
import hal.util;
import scatter_span;
import test_util;

namespace {

constexpr hal::byte write_failure_byte{ 'C' };

class test_serial : public hal::serial
{
public:
  ~test_serial() override = default;

  mem::scatter_span<hal::byte const> m_out{};

private:
  async::future<void> driver_configure(async::context&,
                                       settings const&) override
  {
    return {};
  }

  async::future<void> driver_write(
    async::context&,
    mem::scatter_span<hal::byte const> p_data) override
  {
    m_out = p_data;

    for (auto chunk : p_data) {
      if (not chunk.empty() && chunk[0] == write_failure_byte) {
        throw hal::io_error(this);
      }
    }

    return {};
  }

  hal::circular_span<hal::byte const> driver_receive_buffer() override
  {
    return m_receive_buffer;
  }

  hal::usize driver_receive_cursor() override
  {
    return 0;
  }

  std::array<hal::byte, 1> m_receive_buffer{};
};

async::inplace_context<1024> ctx;

void write_test()
{
  using namespace boost::ut;

  "[success] write"_test = []() {
    // Setup
    test_serial serial;
    std::array<hal::byte, 4> const expected_payload{};
    mem::scatter_array<hal::byte const, 1> payload{ expected_payload };

    // Exercise
    auto future = hal::write(ctx, serial, payload);
    finish(future);

    // Verify
    expect(that % payload == serial.m_out);
  };

  "[failure] write"_test = []() {
    // Setup
    test_serial serial;
    std::array<hal::byte, 4> const expected_payload{ write_failure_byte };
    mem::scatter_array<hal::byte const, 1> payload{ expected_payload };

    // Exercise
    expect(throws<hal::io_error>([&serial, &payload]() {
      auto future = hal::write(ctx, serial, payload);
      finish(future);
    }));
  };

  "[success] write(std::string_view)"_test = []() {
    // Setup
    test_serial serial;
    std::string_view expected = "abcd";

    // Exercise
    auto future = hal::write(ctx, serial, expected);
    finish(future);

    // Verify
    expect(that % 4 == serial.m_out.length());
  };
}

void print_test()
{
  using namespace boost::ut;

  "[printf style] print()"_test = []() {
    // Setup
    test_serial serial;

    // Exercise
    auto future = hal::print<128>(ctx, serial, "hello %d", 5);
    finish(future);

    // Verify
    expect(that % 7 == serial.m_out.length());
  };
}

}  // namespace

int main()
{
  write_test();
  print_test();
}
