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
#include <tuple>
#include <type_traits>

#include <boost/ut.hpp>

import hal;
import hal.util;
import scatter_span;
import test_util;

using namespace mp_units::si::unit_symbols;

namespace {

constexpr hal::byte successful_address{ 0x15 };
constexpr hal::byte failure_address{ 0x33 };
constexpr hal::byte filler_byte{ 0xA5 };

class test_i2c : public hal::i2c
{
public:
  ~test_i2c() = default;

  hal::byte m_address = hal::byte{ 0 };
  mem::scatter_span<hal::byte const> m_out{};
  mem::scatter_span<hal::byte> m_in{};

private:
  void driver_transaction(hal::byte p_address,
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
  }
};

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
    hal::write(i2c, successful_address, payload);

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
    expect(throws<hal::no_such_device>(
      [&i2c, &payload]() { hal::write(i2c, failure_address, payload); }));

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
    hal::read(i2c, successful_address, payload);

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
    expect(throws<hal::no_such_device>(
      [&i2c, &payload]() { hal::read(i2c, failure_address, payload); }));

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
    auto actual = hal::read<expected.size()>(i2c, successful_address);

    // Verify
    expect(successful_address == i2c.m_address);
    expect(std::equal(expected.begin(), expected.end(), actual.begin()));
    expect(that % empty_const_scatter_span == i2c.m_out);
  };

  "[failure] read<Length>"_test = []() {
    // Setup
    test_i2c i2c;

    // Exercise
    expect(throws<hal::no_such_device>(
      [&i2c]() { std::ignore = hal::read<5>(i2c, failure_address); }));

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
    hal::write_then_read(i2c, successful_address, out_payload, in_payload);

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
      hal::write_then_read(i2c, failure_address, out_payload, in_payload);
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
    auto actual = hal::write_then_read<expected.size()>(
      i2c, successful_address, out_payload);

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
      std::ignore = hal::write_then_read<5>(i2c, failure_address, out_payload);
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
    auto exists = hal::probe(i2c, successful_address);

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
    auto exists = hal::probe(i2c, failure_address);

    // Verify
    expect(!exists);
    expect(failure_address == i2c.m_address);
    expect(that % 1 == i2c.m_in.length());
    expect(that % 0 == i2c.m_out.length());
  };
}

/// Counts lock() and unlock() calls and tracks whether the lock is held
class counting_lock : public hal::basic_lock
{
public:
  int locks = 0;
  int unlocks = 0;
  bool held = false;

private:
  void driver_lock() override
  {
    locks++;
    held = true;
  }

  void driver_unlock() override
  {
    unlocks++;
    held = false;
  }
};

struct fast_device
{
  static constexpr hal::hertz max_clock_rate = 400 * kHz;
};

struct slow_device
{
  static constexpr hal::hertz max_clock_rate = 100 * kHz;
};

struct unrated_device
{};

static_assert(hal::i2c_rated_device<fast_device>);
static_assert(not hal::i2c_rated_device<unrated_device>);

void i2c_bus_rate_test()
{
  using namespace boost::ut;

  "[success] i2c_bus_rate picks the slowest device"_test = []() {
    // Exercise
    constexpr auto rate =
      hal::i2c_bus_rate<fast_device, slow_device>(1000 * kHz);

    // Verify
    expect(rate == 100 * kHz);
  };

  "[success] i2c_bus_rate is limited by the board"_test = []() {
    // Exercise
    constexpr auto rate = hal::i2c_bus_rate<fast_device>(200 * kHz);

    // Verify
    expect(rate == 200 * kHz);
  };

  "[success] i2c_bus_rate with no devices is the board limit"_test = []() {
    // Exercise
    constexpr auto rate = hal::i2c_bus_rate<>(400 * kHz);

    // Verify
    expect(rate == 400 * kHz);
  };
}

/// Records whether the lock was held when each call reached the bus
class lock_checking_i2c : public test_i2c
{
public:
  explicit lock_checking_i2c(counting_lock& p_lock)
    : m_lock(&p_lock)
  {
  }

  lock_checking_i2c(lock_checking_i2c const&) = delete;
  lock_checking_i2c& operator=(lock_checking_i2c const&) = delete;
  lock_checking_i2c(lock_checking_i2c&&) = delete;
  lock_checking_i2c& operator=(lock_checking_i2c&&) = delete;
  ~lock_checking_i2c() = default;

  int calls_without_lock = 0;

private:
  void driver_transaction(hal::byte p_address,
                          mem::scatter_span<hal::byte const> p_data_out,
                          mem::scatter_span<hal::byte> p_data_in) override
  {
    if (not m_lock->held) {
      calls_without_lock++;
    }
    m_address = p_address;
    m_out = p_data_out;
    m_in = p_data_in;
    if (p_address == failure_address) {
      throw hal::no_such_device(p_address, this);
    }
  }

  counting_lock* m_lock;
};

void locked_i2c_test()
{
  using namespace boost::ut;

  "[success] locked_i2c holds the lock for a transaction"_test = []() {
    // Setup
    counting_lock lock;
    lock_checking_i2c bus(lock);
    hal::locked_i2c i2c(bus, lock);
    std::array<hal::byte, 2> const out_buffer{ 1, 2 };
    std::array<hal::byte, 3> in_buffer{};
    mem::scatter_array<hal::byte const, 1> out{ out_buffer };
    mem::scatter_array<hal::byte, 1> in{ in_buffer };

    // Exercise
    hal::write_then_read(i2c, successful_address, out, in);

    // Verify
    expect(that % 0 == bus.calls_without_lock);
    expect(that % 1 == lock.locks);
    expect(that % 1 == lock.unlocks);
    expect(successful_address == bus.m_address);
    expect(that % out_buffer.size() == bus.m_out.length());
    expect(that % in_buffer.size() == bus.m_in.length());
  };

  "[failure] locked_i2c releases the lock when the bus throws"_test = []() {
    // Setup
    counting_lock lock;
    lock_checking_i2c bus(lock);
    hal::locked_i2c i2c(bus, lock);

    // Exercise
    expect(throws<hal::no_such_device>(
      [&] { hal::write(i2c, failure_address, {}); }));

    // Verify
    expect(that % 1 == lock.locks);
    expect(that % 1 == lock.unlocks);
    expect(not lock.held);
  };

  "[success] locked_i2c owns a non-movable bus built by a factory"_test = []() {
    // Setup
    counting_lock lock;
    hal::locked_i2c i2c([&lock] { return lock_checking_i2c(lock); }, lock);
    static_assert(
      std::is_same_v<decltype(i2c), hal::locked_i2c<lock_checking_i2c>>);

    // Exercise
    auto const found = hal::probe(i2c, successful_address);
    auto const missing = hal::probe(i2c, failure_address);

    // Verify
    expect(found);
    expect(not missing);
    expect(that % 2 == lock.locks);
    expect(that % 2 == lock.unlocks);
  };
}

}  // namespace

int main()
{
  write_test();
  read_test();
  write_then_read_test();
  probe_test();
  locked_i2c_test();
  i2c_bus_rate_test();
}
