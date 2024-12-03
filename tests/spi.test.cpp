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

#include <libhal-util/spi.hpp>

#include <libhal/error.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"spi"> spi_test = [] {
  using namespace boost::ut;

  static constexpr hal::byte success_filler{ 0xF5 };
  static constexpr hal::byte failure_filler{ 0x33 };
  static constexpr hal::byte filler_byte{ 0xA5 };

  class test_spi : public hal::spi
  {
  public:
    void driver_configure(settings const&) override
    {
    }
    void driver_transfer(std::span<hal::byte const> p_out,
                         std::span<hal::byte> p_in,
                         hal::byte p_filler) override
    {
      if (!p_out.empty()) {
        m_out = p_out;
      }

      if (!p_in.empty()) {
        m_in = p_in;
        std::fill(m_in.begin(), m_in.end(), filler_byte);
      }

      m_filler = p_filler;

      if (p_filler == failure_filler) {
        safe_throw(hal::io_error(this));
      }
    }

    ~test_spi() override = default;

    std::span<hal::byte const> m_out = std::span<hal::byte const>{};
    std::span<hal::byte> m_in = std::span<hal::byte>{};
    hal::byte m_filler = hal::byte{ 0xFF };
  };

  "[success] write"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 4> const expected_payload{};

    // Exercise
    write(spi, expected_payload);

    // Verify
    expect(that % expected_payload.data() == spi.m_out.data());
    expect(that % expected_payload.size() == spi.m_out.size());
    expect(that % nullptr == spi.m_in.data());
    expect(that % 0 == spi.m_in.size());
  };

  "[success] read"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 4> expected_buffer;

    // Exercise
    read(spi, expected_buffer, success_filler);

    // Verify
    expect(success_filler == spi.m_filler);
    expect(that % expected_buffer.data() == spi.m_in.data());
    expect(that % expected_buffer.size() == spi.m_in.size());
    expect(that % nullptr == spi.m_out.data());
    expect(that % 0 == spi.m_out.size());
  };

  "[failure] read"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 4> expected_buffer;

    // Exercise
    expect(throws<hal::io_error>([&spi, &expected_buffer]() {
      read(spi, expected_buffer, failure_filler);
    }))
      << "Exception not thrown!";

    // Verify
    expect(failure_filler == spi.m_filler);
    expect(that % expected_buffer.data() == spi.m_in.data());
    expect(that % expected_buffer.size() == spi.m_in.size());
    expect(that % nullptr == spi.m_out.data());
    expect(that % 0 == spi.m_out.size());
  };

  "[success] read<Length>"_test = []() {
    // Setup
    test_spi spi;
    std::array<hal::byte, 5> expected_buffer;
    expected_buffer.fill(filler_byte);

    // Exercise
    auto actual_buffer = read<expected_buffer.size()>(spi, success_filler);

    // Verify
    expect(success_filler == spi.m_filler);
    expect(std::equal(
      expected_buffer.begin(), expected_buffer.end(), actual_buffer.begin()));
    expect(that % nullptr == spi.m_out.data());
    expect(that % 0 == spi.m_out.size());
  };

#if 0
  "[failure] read<Length>"_test = []() {
    // Setup
    test_spi spi;

    // Exercise
    auto result = read<5>(spi, failure_filler);
    bool successful = static_cast<bool>(result);

    // Verify
    expect(!successful);
    expect(failure_filler == spi.m_filler);
    expect(that % nullptr == spi.m_out.data());
    expect(that % 0 == spi.m_out.size());
  };

  "[success] write_then_read"_test = []() {
    // Setup
    test_spi spi;
    const std::array<hal::byte, 4> expected_payload{};
    std::array<hal::byte, 4> expected_buffer;

    // Exercise
    auto result =
      write_then_read(spi, expected_payload, expected_buffer, success_filler);
    bool successful = static_cast<bool>(result);

    // Verify
    expect(successful);
    expect(success_filler == spi.m_filler);
    expect(that % expected_payload.data() == spi.m_out.data());
    expect(that % expected_payload.size() == spi.m_out.size());
    expect(that % expected_buffer.data() == spi.m_in.data());
    expect(that % expected_buffer.size() == spi.m_in.size());
  };

  "[failure] write_then_read"_test = []() {
    // Setup
    test_spi spi;
    const std::array<hal::byte, 4> expected_payload{};
    std::array<hal::byte, 4> expected_buffer;
    expected_buffer.fill(filler_byte);

    // Exercise
    auto result =
      write_then_read(spi, expected_payload, expected_buffer, failure_filler);
    bool successful = static_cast<bool>(result);

    // Verify
    expect(!successful);
    expect(failure_filler == spi.m_filler);
    expect(that % expected_payload.data() == spi.m_out.data());
    expect(that % expected_payload.size() == spi.m_out.size());
    expect(that % expected_buffer.data() == spi.m_in.data());
    expect(that % expected_buffer.size() == spi.m_in.size());
  };

  "[success] write_then_read<Length>"_test = []() {
    // Setup
    test_spi spi;
    const std::array<hal::byte, 4> expected_payload{};
    std::array<hal::byte, 4> expected_buffer{};
    expected_buffer.fill(filler_byte);

    // Exercise
    auto result = write_then_read<5>(spi, expected_payload, success_filler);
    bool successful = static_cast<bool>(result);
    auto actual_array = result.value();

    // Verify
    expect(successful);
    expect(success_filler == spi.m_filler);
    expect(that % expected_payload.data() == spi.m_out.data());
    expect(that % expected_payload.size() == spi.m_out.size());
    expect(std::equal(
      expected_buffer.begin(), expected_buffer.end(), actual_array.begin()));
  };

  "[failure] write_then_read<Length>"_test = []() {
    // Setup
    test_spi spi;
    const std::array<hal::byte, 4> expected_payload{};

    // Exercise
    auto result = write_then_read<5>(spi, expected_payload, failure_filler);
    bool successful = static_cast<bool>(result);

    // Verify
    expect(!successful);
    expect(failure_filler == spi.m_filler);
    expect(that % expected_payload.data() == spi.m_out.data());
    expect(that % expected_payload.size() == spi.m_out.size());
  };
#endif
};
}  // namespace hal
