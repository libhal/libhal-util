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

#include <libhal-util/serial.hpp>

#include <algorithm>
#include <array>
#include <memory_resource>
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

  class fake_v5_serial : public hal::v5::serial
  {
  public:
    fake_v5_serial(std::pmr::polymorphic_allocator<hal::byte> p_allocator,
                   hal::usize p_buffer_size = 16)
      : m_allocator(p_allocator)
      , m_receive_buffer(p_buffer_size, hal::byte{ 0 }, p_allocator)
    {
    }

    void driver_configure(settings const& p_settings) override
    {
      m_last_settings = p_settings;
      m_configure_called = true;
    }

    void driver_write(std::span<hal::byte const> p_data) override
    {
      m_write_call_count++;
      if (!p_data.empty() && p_data[0] == write_failure_byte) {
        hal::safe_throw(hal::io_error(this));
      }
      m_written_data.assign(p_data.begin(), p_data.end());
    }

    std::span<hal::byte const> driver_receive_buffer() override
    {
      return m_receive_buffer;
    }

    hal::usize driver_cursor() override
    {
      return m_cursor;
    }

    // Test helpers
    void add_received_data(std::span<hal::byte const> p_data)
    {
      for (auto byte : p_data) {
        m_receive_buffer[m_cursor] = byte;
        m_cursor = (m_cursor + 1) % m_receive_buffer.size();
      }
    }

    void reset()
    {
      m_cursor = 0;
      m_write_call_count = 0;
      m_configure_called = false;
      m_written_data.clear();
      std::ranges::fill(m_receive_buffer, hal::byte{ 0 });
    }

    ~fake_v5_serial() override = default;

    std::pmr::polymorphic_allocator<hal::byte> m_allocator;
    std::pmr::vector<hal::byte> m_receive_buffer;
    std::pmr::vector<hal::byte> m_written_data;
    hal::usize m_cursor = 0;
    int m_write_call_count = 0;
    bool m_configure_called = false;
    hal::v5::serial::settings m_last_settings{};
  };

  "serial_v5_to_legacy_converter"_test = []() {
    auto* allocator = std::pmr::new_delete_resource();

    "[success] constructor and basic setup"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);

      // Exercise
      auto converter = hal::make_serial_converter(allocator, v5_serial);

      // Verify
      auto const inner = converter->v5_serial();
      expect(that % &(*inner) == &(*v5_serial));
    };

    "[success] configure settings conversion"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      auto converter = hal::make_serial_converter(allocator, v5_serial);

      hal::serial::settings legacy_settings{
        .baud_rate = 9600,
        .stop = hal::serial::settings::stop_bits::two,
        .parity = hal::serial::settings::parity::even
      };

      // Exercise
      converter->configure(legacy_settings);

      // Verify
      expect(that % v5_serial->m_configure_called);
      expect(that % 9600 == v5_serial->m_last_settings.baud_rate);
      expect(hal::v5::serial::settings::stop_bits::two ==
             v5_serial->m_last_settings.stop);
      expect(hal::v5::serial::settings::parity::even ==
             v5_serial->m_last_settings.parity);
    };

    "[success] write operation"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      auto converter = hal::make_serial_converter(allocator, v5_serial);
      std::array<hal::byte, 4> test_data{
        hal::byte{ 'A' }, hal::byte{ 'B' }, hal::byte{ 'C' }, hal::byte{ 'D' }
      };

      // Exercise
      auto result = converter->write(test_data);

      // Verify
      expect(that % 1 == v5_serial->m_write_call_count);
      expect(that % test_data.size() == v5_serial->m_written_data.size());
      expect(that % test_data.size() == result.data.size());
      expect(std::equal(
        test_data.begin(), test_data.end(), v5_serial->m_written_data.begin()));
    };

    "[failure] write operation with error"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      auto converter = hal::make_serial_converter(allocator, v5_serial);
      std::array<hal::byte, 1> test_data{ write_failure_byte };

      // Exercise & Verify
      expect(throws<hal::io_error>([&]() { converter->write(test_data); }));
    };

    "[success] read operation - no wraparound"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      auto converter = hal::make_serial_converter(allocator, v5_serial);

      std::array<hal::byte, 4> test_data{
        hal::byte{ 'A' }, hal::byte{ 'B' }, hal::byte{ 'C' }, hal::byte{ 'D' }
      };
      v5_serial->add_received_data(test_data);

      std::array<hal::byte, 4> read_buffer{};

      // Exercise
      auto result = converter->read(read_buffer);

      // Verify
      expect(that % 4 == result.data.size());
      expect(that % 0 == result.available);
      expect(that % 16 == result.capacity);
      expect(
        std::equal(test_data.begin(), test_data.end(), read_buffer.begin()));
    };

    "[success] read operation - with wraparound"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 8);
      auto converter = hal::make_serial_converter(allocator, v5_serial);

      // Fill buffer to near end, then add data that wraps around
      std::array<hal::byte, 6> initial_data{
        hal::byte{ '1' }, hal::byte{ '2' }, hal::byte{ '3' },
        hal::byte{ '4' }, hal::byte{ '5' }, hal::byte{ '6' }
      };
      v5_serial->add_received_data(initial_data);

      // Read some data to advance our tracking cursor
      std::array<hal::byte, 4> temp_buffer{};
      [[maybe_unused]] auto const first_read = converter->read(temp_buffer);

      // Add more data that will wrap around
      std::array<hal::byte, 4> wrap_data{
        hal::byte{ 'A' }, hal::byte{ 'B' }, hal::byte{ 'C' }, hal::byte{ 'D' }
      };
      v5_serial->add_received_data(wrap_data);

      std::array<hal::byte, 6> read_buffer{};

      // Exercise
      auto result = converter->read(read_buffer);

      // Verify - Should read the remaining initial data + wrap data
      expect(that % 6 == result.data.size());
      expect(that % read_buffer[0] ==
             hal::byte{ '5' });  // remaining from initial
      expect(that % read_buffer[1] ==
             hal::byte{ '6' });  // remaining from initial
      expect(that % read_buffer[2] == hal::byte{ 'A' });  // wrapped data
      expect(that % read_buffer[5] == hal::byte{ 'D' });  // wrapped data
    };

    "[success] partial read"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      auto converter = hal::make_serial_converter(allocator, v5_serial);

      std::array<hal::byte, 6> test_data{ hal::byte{ 'A' }, hal::byte{ 'B' },
                                          hal::byte{ 'C' }, hal::byte{ 'D' },
                                          hal::byte{ 'E' }, hal::byte{ 'F' } };
      v5_serial->add_received_data(test_data);

      std::array<hal::byte, 3> read_buffer{};

      // Exercise
      auto result = converter->read(read_buffer);

      // Verify
      expect(that % 3 == result.data.size());
      expect(that % 3 == result.available);  // 3 more bytes still available
      expect(that % read_buffer[0] == hal::byte{ 'A' });
      expect(that % read_buffer[2] == hal::byte{ 'C' });
    };

    "[success] flush operation"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      auto converter = hal::make_serial_converter(allocator, v5_serial);

      std::array<hal::byte, 4> test_data{
        hal::byte{ 'A' }, hal::byte{ 'B' }, hal::byte{ 'C' }, hal::byte{ 'D' }
      };
      v5_serial->add_received_data(test_data);

      std::array<hal::byte, 2> read_buffer{};

      // Exercise - read some data first
      auto first_result = converter->read(read_buffer);
      expect(that % 2 == first_result.data.size());
      expect(that % 2 == first_result.available);

      // Now flush
      converter->flush();

      // Try to read again
      auto second_result = converter->read(read_buffer);

      // Verify - should have no data available after flush
      expect(that % 0 == second_result.data.size());
      expect(that % 0 == second_result.available);
    };
  };

  "v5::serial utilities"_test = []() {
    auto* allocator = std::pmr::new_delete_resource();

    "[success] v5::write with bytes"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      std::array<hal::byte, 4> test_data{
        hal::byte{ 'A' }, hal::byte{ 'B' }, hal::byte{ 'C' }, hal::byte{ 'D' }
      };

      // Exercise
      hal::v5::write(v5_serial, test_data);

      // Verify
      expect(that % 1 == v5_serial->m_write_call_count);
      expect(that % test_data.size() == v5_serial->m_written_data.size());
      expect(std::equal(
        test_data.begin(), test_data.end(), v5_serial->m_written_data.begin()));
    };

    "[success] v5::write with string_view"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      std::string_view test_string = "Hello";

      // Exercise
      hal::v5::write(v5_serial, test_string);

      // Verify
      expect(that % 1 == v5_serial->m_write_call_count);
      expect(that % test_string.size() == v5_serial->m_written_data.size());
      expect(that % hal::byte{ 'H' } == v5_serial->m_written_data[0]);
      expect(that % hal::byte{ 'e' } == v5_serial->m_written_data[1]);
      expect(that % hal::byte{ 'l' } == v5_serial->m_written_data[2]);
      expect(that % hal::byte{ 'l' } == v5_serial->m_written_data[3]);
      expect(that % hal::byte{ 'o' } == v5_serial->m_written_data[4]);
    };

    "[success] v5::print with string_view"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 16);
      std::string_view test_string = "World";

      // Exercise
      hal::v5::print(v5_serial, test_string);

      // Verify
      expect(that % 1 == v5_serial->m_write_call_count);
      expect(that % test_string.size() == v5_serial->m_written_data.size());
      expect(that % hal::byte{ 'W' } == v5_serial->m_written_data[0]);
      expect(that % hal::byte{ 'o' } == v5_serial->m_written_data[1]);
      expect(that % hal::byte{ 'r' } == v5_serial->m_written_data[2]);
      expect(that % hal::byte{ 'l' } == v5_serial->m_written_data[3]);
      expect(that % hal::byte{ 'd' } == v5_serial->m_written_data[4]);
    };

    "[success] v5::print with printf format"_test = [&]() {
      // Setup
      auto v5_serial =
        hal::v5::make_strong_ptr<fake_v5_serial>(allocator, allocator, 32);

      // Exercise
      hal::v5::print<32>(v5_serial, "Value: %d", 42);

      // Verify
      expect(that % 1 == v5_serial->m_write_call_count);
      std::string_view expected = "Value: 42";
      expect(that % expected.size() == v5_serial->m_written_data.size());

      std::string actual(
        reinterpret_cast<char const*>(v5_serial->m_written_data.data()),
        v5_serial->m_written_data.size());
      expect(that % expected == actual);
    };
  };
};
}  // namespace hal
