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

#include <libhal-util/i2c.hpp>

#include <vector>

#include <libhal/error.hpp>
#include <libhal/functional.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"i2c_test"> i2c_test = [] {
  using namespace boost::ut;

  static constexpr hal::byte successful_address{ 0x15 };
  static constexpr hal::byte failure_address{ 0x33 };
  static constexpr hal::byte filler_byte{ 0xA5 };

  struct test_timeout_t
  {
    void operator()()
    {
      was_called = true;
    }
    bool was_called = false;
  };

  class test_i2c : public hal::i2c
  {
  public:
    void driver_configure(settings const&) override
    {
    }

    void driver_transaction(
      hal::byte p_address,
      std::span<hal::byte const> p_out,
      std::span<hal::byte> p_in,
      hal::function_ref<hal::timeout_function> p_timeout) override
    {
      m_address = p_address;
      m_out = p_out;
      m_in = p_in;

      std::ranges::fill(m_in, filler_byte);

      if (m_address == failure_address) {
        safe_throw(hal::no_such_device(m_address, this));
      }

      p_timeout();
    }

    ~test_i2c() override = default;

    hal::byte m_address = hal::byte{ 0 };
    std::span<hal::byte const> m_out = std::span<hal::byte const>{};
    std::span<hal::byte> m_in = std::span<hal::byte>{};
  };

  "operator==(i2c::settings)"_test = []() {
    i2c::settings a{};
    i2c::settings b{};

    expect(a == b);
  };

  "operator!=(i2c::settings)"_test = []() {
    i2c::settings a{ .clock_rate = 100.0_kHz };
    i2c::settings b{ .clock_rate = 1200.0_kHz };

    expect(a != b);
  };

  "[success] write"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> const expected_payload{};

    // Exercise
    write(i2c, successful_address, expected_payload, std::ref(test_timeout));

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % expected_payload.data() == i2c.m_out.data());
    expect(that % expected_payload.size() == i2c.m_out.size());
    expect(that % nullptr == i2c.m_in.data());
    expect(that % 0 == i2c.m_in.size());
    // Verify: timeout will be ignored from libhal 4.5.0 and beyond
    expect(that % false == test_timeout.was_called);
  };

  "[failure] write"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> const expected_payload{};

    // Exercise
    expect(
      throws<hal::no_such_device>([&i2c, &expected_payload, &test_timeout]() {
        write(i2c, failure_address, expected_payload, std::ref(test_timeout));
      }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % expected_payload.data() == i2c.m_out.data());
    expect(that % expected_payload.size() == i2c.m_out.size());
    expect(that % nullptr == i2c.m_in.data());
    expect(that % 0 == i2c.m_in.size());
    expect(that % false == test_timeout.was_called);
  };

  "[success] read"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> expected_buffer;

    // Exercise
    read(i2c, successful_address, expected_buffer, std::ref(test_timeout));

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % expected_buffer.data() == i2c.m_in.data());
    expect(that % expected_buffer.size() == i2c.m_in.size());
    expect(that % nullptr == i2c.m_out.data());
    expect(that % 0 == i2c.m_out.size());
    // Verify: timeout will be ignored from libhal 4.5.0 and beyond
    expect(that % false == test_timeout.was_called);
  };

  "[failure] read"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> expected_buffer;

    // Exercise
    expect(
      throws<hal::no_such_device>([&i2c, &expected_buffer, &test_timeout]() {
        read(i2c, failure_address, expected_buffer, std::ref(test_timeout));
      }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % expected_buffer.data() == i2c.m_in.data());
    expect(that % expected_buffer.size() == i2c.m_in.size());
    expect(that % nullptr == i2c.m_out.data());
    expect(that % 0 == i2c.m_out.size());
    expect(that % false == test_timeout.was_called);
  };

  "[success] read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 5> expected;
    expected.fill(filler_byte);

    // Exercise
    auto actual =
      read<expected.size()>(i2c, successful_address, std::ref(test_timeout));

    // Verify
    expect(successful_address == i2c.m_address);
    expect(std::equal(expected.begin(), expected.end(), actual.begin()));
    expect(that % nullptr == i2c.m_out.data());
    expect(that % 0 == i2c.m_out.size());
    // Verify: timeout will be ignored from libhal 4.5.0 and beyond
    expect(that % false == test_timeout.was_called);
  };

  "[failure] read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;

    // Exercise
    expect(throws<hal::no_such_device>([&i2c]() {
      [[maybe_unused]] auto result =
        read<5>(i2c, failure_address, never_timeout());
    }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % nullptr == i2c.m_out.data());
    expect(that % 0 == i2c.m_out.size());
    expect(that % false == test_timeout.was_called);
  };

  "[success] write_then_read"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> const expected_payload{};
    std::array<hal::byte, 4> expected_buffer;

    // Exercise
    write_then_read(i2c,
                    successful_address,
                    expected_payload,
                    expected_buffer,
                    std::ref(test_timeout));

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % expected_payload.data() == i2c.m_out.data());
    expect(that % expected_payload.size() == i2c.m_out.size());
    expect(that % expected_buffer.data() == i2c.m_in.data());
    expect(that % expected_buffer.size() == i2c.m_in.size());
    // Verify: timeout will be ignored from libhal 4.5.0 and beyond
    expect(that % false == test_timeout.was_called);
  };

  "[failure] write_then_read"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> const expected_payload{};
    std::array<hal::byte, 4> expected_buffer;
    expected_buffer.fill(filler_byte);

    // Exercise
    expect(throws<hal::no_such_device>(
      [&i2c, &expected_payload, &expected_buffer, &test_timeout]() {
        write_then_read(i2c,
                        failure_address,
                        expected_payload,
                        expected_buffer,
                        std::ref(test_timeout));
      }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % expected_payload.data() == i2c.m_out.data());
    expect(that % expected_payload.size() == i2c.m_out.size());
    expect(that % expected_buffer.data() == i2c.m_in.data());
    expect(that % expected_buffer.size() == i2c.m_in.size());
    expect(that % false == test_timeout.was_called);
  };

  "[success] write_then_read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> const expected_payload{};
    std::array<hal::byte, 4> expected{};
    expected.fill(filler_byte);

    // Exercise
    auto actual = write_then_read<5>(
      i2c, successful_address, expected_payload, std::ref(test_timeout));

    // Verify
    expect(successful_address == i2c.m_address);
    expect(that % expected_payload.data() == i2c.m_out.data());
    expect(that % expected_payload.size() == i2c.m_out.size());
    expect(std::equal(expected.begin(), expected.end(), actual.begin()));
    // Verify: timeout will be ignored from libhal 4.5.0 and beyond
    expect(that % false == test_timeout.was_called);
  };

  "[failure] write_then_read<Length>"_test = []() {
    // Setup
    test_i2c i2c;
    test_timeout_t test_timeout;
    std::array<hal::byte, 4> const expected_payload{};

    // Exercise
    expect(
      throws<hal::no_such_device>([&i2c, &expected_payload, &test_timeout]() {
        [[maybe_unused]] auto result = write_then_read<5>(
          i2c, failure_address, expected_payload, std::ref(test_timeout));
      }));

    // Verify
    expect(failure_address == i2c.m_address);
    expect(that % expected_payload.data() == i2c.m_out.data());
    expect(that % expected_payload.size() == i2c.m_out.size());
    expect(that % false == test_timeout.was_called);
  };

  "[success] probe(i2c&)"_test = []() {
    // Setup
    test_i2c i2c;

    // Exercise
    auto exists = probe(i2c, successful_address);

    // Verify
    expect(exists);
    expect(successful_address == i2c.m_address);
    expect(that % 1 == i2c.m_in.size());
    expect(that % nullptr != i2c.m_in.data());
    expect(that % 0 == i2c.m_out.size());
    expect(that % nullptr == i2c.m_out.data());
  };

  "[failure] probe(i2c&)"_test = []() {
    // Setup
    test_i2c i2c;

    // Exercise
    auto exists = probe(i2c, failure_address);

    // Verify
    expect(!exists);
    expect(failure_address == i2c.m_address);
    expect(that % 1 == i2c.m_in.size());
    expect(that % nullptr != i2c.m_in.data());
    expect(that % 0 == i2c.m_out.size());
    expect(that % nullptr == i2c.m_out.data());
  };

  "Use all APIs without timeout parameter"_test = []() {
    // Setup
    test_i2c i2c;

    std::array<hal::byte, 4> const write_data{};
    std::array<hal::byte, 4> read_data{};

    // Exercise
    // Verify
    write(i2c, successful_address, write_data);
    read(i2c, successful_address, read_data);
    write_then_read(i2c, successful_address, write_data, read_data);
    [[maybe_unused]] auto r_array = read<2>(i2c, successful_address);
    [[maybe_unused]] auto wr_array =
      write_then_read<2>(i2c, successful_address, write_data);
  };
};
}  // namespace hal
