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

#include <libhal-util/serial.hpp>

#include <algorithm>
#include <array>
#include <span>

#include <libhal-util/comparison.hpp>
#include <libhal/error.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"serial_test"> serial_test = [] {
  using namespace boost::ut;

  static constexpr hal::byte write_failure_byte{ 'C' };
  static constexpr hal::byte filler_byte{ 'A' };

  class fake_serial : public hal::serial
  {
  public:
    void driver_configure(settings const&) override
    {
    }

    write_t driver_write(std::span<hal::byte const> p_data) override
    {
      m_write_call_count++;
      if (p_data[0] == write_failure_byte) {
        safe_throw(hal::io_error(this));
      }
      m_out = p_data;

      if (m_single_byte_out) {
        return write_t{ p_data.subspan(0, 1) };
      }
      return write_t{ p_data };
    }

    read_t driver_read(std::span<hal::byte> p_data) override
    {
      if (p_data.size() == 0) {
        return read_t{
          .data = p_data,
          .available = 1,
          .capacity = 1,
        };
      }

      m_read_was_called = true;

      if (m_read_fails) {
        safe_throw(hal::io_error(this));
      }

      // only fill 1 byte at a time
      p_data[0] = filler_byte;

      return read_t{
        .data = p_data.subspan(0, 1),
        .available = 1,
        .capacity = 1,
      };
    }

    void driver_flush() override
    {
      m_flush_called = true;
    }

    ~fake_serial() override = default;

    std::span<hal::byte const> m_out{};
    int m_write_call_count = 0;
    bool m_read_was_called = false;
    bool m_flush_called = false;
    bool m_read_fails = false;
    bool m_single_byte_out = false;
  };

  "operator==(serial::settings)"_test = []() {
    serial::settings a{};
    serial::settings b{};

    expect(a == b);
  };

  "operator!=(serial::settings)"_test = []() {
    serial::settings a{ .baud_rate = 9600 };
    serial::settings b{ .baud_rate = 1200 };

    expect(a != b);
  };

  "serial/util"_test = []() {
    "[success] write_partial full"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{};

      // Exercise
      auto result = write_partial(serial, expected);

      // Verify
      expect(result.data.size() == expected.size());
      expect(!serial.m_flush_called);
      expect(that % expected.data() == serial.m_out.data());
      expect(that % expected.size() == serial.m_out.size());
      expect(that % !serial.m_read_was_called);
    };

    "[success] write_partial single byte at a time"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{};
      serial.m_single_byte_out = true;

      // Exercise
      auto result = write_partial(serial, expected);

      // Verify
      expect(1 == result.data.size());
      expect(!serial.m_flush_called);
      expect(that % &expected[0] == serial.m_out.data());
      expect(that % 4 == serial.m_out.size());
      expect(that % !serial.m_read_was_called);
    };

    "[failure] write_partial"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{ write_failure_byte };

      // Exercise
      expect(throws<hal::io_error>([&serial, &expected]() {
        [[maybe_unused]] auto result = write_partial(serial, expected);
      }));

      // Verify
      expect(!serial.m_flush_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
      expect(that % !serial.m_read_was_called);
    };

    "[success] write"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{};
      serial.m_single_byte_out = true;

      // Exercise
      write(serial, expected, hal::never_timeout());

      // Verify
      expect(!serial.m_flush_called);
      expect(that % 1 == serial.m_out.size());
      expect(that % expected.size() == serial.m_write_call_count);
      expect(that % !serial.m_read_was_called);
    };

    "[success] write(std::string_view)"_test = []() {
      // Setup
      fake_serial serial;
      std::string_view expected = "abcd";
      serial.m_single_byte_out = true;

      // Exercise
      write(serial, expected, hal::never_timeout());

      // Verify
      expect(!serial.m_flush_called);
      expect(that % expected.end()[-1] == serial.m_out[0]);
      expect(that % 1 == serial.m_out.size());
      expect(that % expected.size() == serial.m_write_call_count);
      expect(that % !serial.m_read_was_called);
    };

    "[success] read"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> expected{};
      expected.fill(filler_byte);
      std::array<hal::byte, 4> actual;
      actual.fill(0);

      // Exercise
      read(serial, actual, never_timeout());

      // Verify
      expect(!serial.m_flush_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
      expect(that % expected == actual);
    };

    "[failure read] read"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> actual{};
      serial.m_read_fails = true;

      // Exercise
      expect(throws<hal::io_error>(
        [&serial, &actual]() { read(serial, actual, never_timeout()); }));

      // Verify
      expect(!serial.m_flush_called);
      expect(that % serial.m_read_was_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
    };

    "[success] read<Length>"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 5> expected{};
      expected.fill(filler_byte);

      // Exercise
      auto actual = read<expected.size()>(serial, never_timeout());

      // Verify
      expect(!serial.m_flush_called);
      expect(that % expected == actual);
      expect(that % serial.m_read_was_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
      expect(that % expected == actual);
    };

    "[failure read] read<Length>"_test = []() {
      // Setup
      fake_serial serial;
      serial.m_read_fails = true;

      // Exercise
      expect(throws<hal::io_error>([&serial]() {
        [[maybe_unused]] auto result = read<5>(serial, never_timeout());
      }));

      // Verify
      expect(!serial.m_flush_called);
      expect(that % serial.m_read_was_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
    };

    "[success] write_then_read"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const write_buffer{};
      std::array<hal::byte, 4> expected_read{};
      expected_read.fill(filler_byte);
      std::array<hal::byte, 4> actual;
      actual.fill(0);

      // Exercise
      write_then_read(serial, write_buffer, actual, never_timeout());

      // Verify
      expect(!serial.m_flush_called);
      expect(that % write_buffer.data() == serial.m_out.data());
      expect(that % write_buffer.size() == serial.m_out.size());
      expect(that % expected_read == actual);
    };

    "[failure read] write_then_read"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{};
      std::array<hal::byte, 4> actual{};
      actual.fill(filler_byte);
      std::array<hal::byte, 4> actual_buffer;
      actual_buffer.fill(0);
      serial.m_read_fails = true;

      // Exercise
      expect(throws<hal::io_error>([&serial, &expected, &actual_buffer]() {
        write_then_read(serial, expected, actual_buffer, never_timeout());
      }));

      // Verify
      expect(!serial.m_flush_called);
      expect(that % serial.m_read_was_called);
      expect(that % expected.data() == serial.m_out.data());
      expect(that % expected.size() == serial.m_out.size());
      expect(that % actual != actual_buffer);
    };

    "[failure on write] write_then_read"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{ write_failure_byte };
      std::array<hal::byte, 4> actual{};

      // Exercise
      expect(throws<hal::io_error>([&serial, &expected, &actual]() {
        write_then_read(serial, expected, actual, never_timeout());
      }));

      // Verify
      expect(!serial.m_flush_called);
      expect(that % !serial.m_read_was_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
    };

    "[success] write_then_read<Length>"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected_write{};
      std::array<hal::byte, 5> expected_read{};
      expected_read.fill(filler_byte);

      // Exercise
      auto actual = write_then_read<5>(serial, expected_write, never_timeout());

      // Verify
      expect(!serial.m_flush_called);
      expect(that % expected_write.data() == serial.m_out.data());
      expect(that % expected_write.size() == serial.m_out.size());
      expect(serial.m_read_was_called);
      expect(that % expected_read == actual);
    };

    "[failure on write] write_then_read<Length>"_test = []() {
      // Setup
      fake_serial serial;
      std::array<hal::byte, 4> const expected{ write_failure_byte };

      // Exercise
      expect(throws<hal::io_error>([&serial, &expected]() {
        [[maybe_unused]] auto result =
          write_then_read<5>(serial, expected, never_timeout());
      }));

      // Verify
      expect(!serial.m_flush_called);
      expect(that % !serial.m_read_was_called);
      expect(that % nullptr == serial.m_out.data());
      expect(that % 0 == serial.m_out.size());
    };
  };

  struct save_serial_write : public hal::serial
  {
    void driver_configure(settings const&) override
    {
    }

    write_t driver_write(std::span<hal::byte const> p_data) override
    {
      m_out.assign(p_data.begin(), p_data.end());
      return write_t{ p_data };
    }

    read_t driver_read(std::span<hal::byte>) override
    {
      safe_throw(hal::io_error(this));

      return {
        .data{},
        .available{},
        .capacity{},
      };
    }

    void driver_flush() override
    {
    }

    ~save_serial_write() override = default;

    std::vector<hal::byte> m_out{};
  };

  "print()"_test = []() {
    "print()"_test = []() {
      // Setup
      save_serial_write serial;
      std::string_view const expected = "hello, world!";

      // Exercise
      print(serial, expected);

      // Verify
      expect(that % expected == std::string_view(reinterpret_cast<char const*>(
                                                   serial.m_out.data()),
                                                 serial.m_out.size()));
    };

    "[printf style 1] print()"_test = []() {
      // Setup
      save_serial_write serial;
      std::string_view const expected = "hello 5";

      // Exercise
      print<128>(serial, "hello %d", 5);

      // Verify
      expect(that % expected == std::string_view(reinterpret_cast<char const*>(
                                                   serial.m_out.data()),
                                                 serial.m_out.size()));
    };

    "[printf style 2] print()"_test = []() {
      // Setup
      save_serial_write serial;
      std::string_view const expected = "hello 5 ABCDEF";

      // Exercise
      print<128>(serial, "hello %d %06X", 5, 0xABCDEF);

      // Verify
      expect(that % expected == std::string_view(reinterpret_cast<char const*>(
                                                   serial.m_out.data()),
                                                 serial.m_out.size()));
    };
  };
};
}  // namespace hal
