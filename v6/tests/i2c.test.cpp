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

#include <boost/ut.hpp>

import hal;
import hal.util;
import scatter_span;
import test_util;

namespace {

constexpr hal::byte successful_address{ 0x15 };
constexpr hal::byte failure_address{ 0x33 };
constexpr hal::byte filler_byte{ 0xA5 };

class test_i2c : public hal::i2c
{
public:
  ~test_i2c() override = default;

  hal::byte m_address = hal::byte{ 0 };
  mem::scatter_span<hal::byte const> m_out{};
  mem::scatter_span<hal::byte> m_in{};

private:
  async::future<void> driver_configure(async::context&,
                                       settings const&) override
  {
    return {};
  }

  async::future<void> driver_transaction(
    async::context&,
    hal::byte p_address,
    mem::scatter_span<hal::byte const> p_data_out,
    mem::scatter_span<hal::byte> p_data_in) override
  {
    m_address = p_address;
    m_out = p_data_out;
    m_in = p_data_in;

    for (auto chunk : p_data_in) {
      for (auto& elem : chunk) {
        elem = filler_byte;
      }
    }

    if (m_address == failure_address) {
      throw hal::no_such_device(m_address, this);
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
    test_i2c i2c;
    std::array<hal::byte, 4> const expected_payload{};
    mem::scatter_array<hal::byte const, 1> payload{ expected_payload };

    // Exercise
    auto future = hal::write(ctx, i2c, successful_address, payload);
    finish(future);

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % payload == i2c.m_out);
    expect(that % empty_scatter_span == i2c.m_in);
  };

  "[failure] write"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> const expected_payload{};
    mem::scatter_array<hal::byte const, 1> payload{ expected_payload };

    // Exercise
    expect(throws<hal::no_such_device>([&i2c, &payload]() {
      auto future = hal::write(ctx, i2c, failure_address, payload);
      finish(future);
    }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % payload == i2c.m_out);
    expect(that % empty_scatter_span == i2c.m_in);
  };
}

void read_test()
{
  using namespace boost::ut;

  "[success] read"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> expected_buffer;
    mem::scatter_array<hal::byte, 1> payload{ expected_buffer };

    // Exercise
    auto future = hal::read(ctx, i2c, successful_address, payload);
    finish(future);

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % payload == i2c.m_in);
    expect(that % empty_const_scatter_span == i2c.m_out);
  };

  "[failure] read"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> expected_buffer;
    mem::scatter_array<hal::byte, 1> payload{ expected_buffer };

    // Exercise
    expect(throws<hal::no_such_device>([&i2c, &payload]() {
      auto future = hal::read(ctx, i2c, failure_address, payload);
      finish(future);
    }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % payload == i2c.m_in);
    expect(that % empty_const_scatter_span == i2c.m_out);
  };

  "[success] read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 5> expected;
    expected.fill(filler_byte);

    // Exercise
    auto future = hal::read<expected.size()>(ctx, i2c, successful_address);
    finish(future);
    auto actual = future.value();

    // Verify
    expect(successful_address == i2c.m_address);
    expect(std::equal(expected.begin(), expected.end(), actual.begin()));
    expect(that % empty_const_scatter_span == i2c.m_out);
  };

  "[failure] read<Length>"_test = []() {
    // Setup
    test_i2c i2c;

    // Exercise
    expect(throws<hal::no_such_device>([&i2c]() {
      auto future = hal::read<5>(ctx, i2c, failure_address);
      finish(future);
    }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % empty_const_scatter_span == i2c.m_out);
  };
}

void write_then_read_test()
{
  using namespace boost::ut;

  "[success] write_then_read"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> const expected_payload{};
    std::array<hal::byte, 4> expected_buffer;
    mem::scatter_array<hal::byte const, 1> out_payload{ expected_payload };
    mem::scatter_array<hal::byte, 1> in_payload{ expected_buffer };

    // Exercise
    auto future = hal::write_then_read(
      ctx, i2c, successful_address, out_payload, in_payload);
    finish(future);

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % out_payload == i2c.m_out);
    expect(that % in_payload == i2c.m_in);
  };

  "[failure] write_then_read"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> const expected_payload{};
    std::array<hal::byte, 4> expected_buffer;
    expected_buffer.fill(filler_byte);
    mem::scatter_array<hal::byte const, 1> out_payload{ expected_payload };
    mem::scatter_array<hal::byte, 1> in_payload{ expected_buffer };

    // Exercise
    expect(throws<hal::no_such_device>([&i2c, &out_payload, &in_payload]() {
      auto future = hal::write_then_read(
        ctx, i2c, failure_address, out_payload, in_payload);
      finish(future);
    }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % out_payload == i2c.m_out);
    expect(that % in_payload == i2c.m_in);
  };

  "[success] write_then_read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> const expected_payload{};
    std::array<hal::byte, 4> expected{};
    expected.fill(filler_byte);
    mem::scatter_array<hal::byte const, 1> out_payload{ expected_payload };

    // Exercise
    auto future = hal::write_then_read<expected.size()>(
      ctx, i2c, successful_address, out_payload);
    finish(future);
    auto actual = future.value();

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % out_payload == i2c.m_out);
    expect(std::equal(expected.begin(), expected.end(), actual.begin()));
  };

  "[failure] write_then_read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    std::array<hal::byte, 4> const expected_payload{};
    mem::scatter_array<hal::byte const, 1> out_payload{ expected_payload };

    // Exercise
    expect(throws<hal::no_such_device>([&i2c, &out_payload]() {
      auto future =
        hal::write_then_read<5>(ctx, i2c, failure_address, out_payload);
      finish(future);
    }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % out_payload == i2c.m_out);
  };
}

void probe_test()
{
  using namespace boost::ut;

  "[success] probe(i2c&)"_test = []() {
    // Setup
    test_i2c i2c;

    // Exercise
    auto future = hal::probe(ctx, i2c, successful_address);
    finish(future);
    auto exists = future.value();

    // Verify
    expect(exists);
    expect(successful_address == i2c.m_address);
    expect(that % 1 == i2c.m_in.length());
    expect(that % 0 == i2c.m_out.length());
  };

  "[failure] probe(i2c&)"_test = []() {
    // Setup
    test_i2c i2c;

    // Exercise
    auto future = hal::probe(ctx, i2c, failure_address);
    finish(future);
    auto exists = future.value();

    // Verify
    expect(!exists);
    expect(failure_address == i2c.m_address);
    expect(that % 1 == i2c.m_in.length());
    expect(that % 0 == i2c.m_out.length());
  };
}

}  // namespace

int main()
{
  write_test();
  read_test();
  write_then_read_test();
  probe_test();
}
