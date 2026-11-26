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

#include <boost/ut.hpp>

import std;
import hal;
import hal.util;

namespace {

void bit_modify_compile_time_test()
{
  using namespace boost::ut;

  "hal::bit_modify<template> compile time masks APIs"_test = []() {
    // Setup
    std::uint32_t volatile control_register = (1U << 15U) | (1U << 16U);
    constexpr auto enable_bit = hal::bit_mask::from(1);
    constexpr auto high_power_mode = hal::bit_mask::from(15);
    constexpr auto clock_divider = hal::bit_mask::from(20, 23);
    constexpr auto phase_delay = hal::bit_mask::from(24, 27);
    constexpr auto extractor_mask = hal::bit_mask::from(16, 23);
    constexpr auto single_bit_mask = hal::bit_mask::from(1);

    // Exercise
    hal::bit_modify(control_register)
      .set<enable_bit>()
      .clear<high_power_mode>()
      .insert<clock_divider>(0xAU)
      .insert<phase_delay, 0x3U>();
    auto extracted = hal::bit_extract<extractor_mask>(control_register);
    auto probed = hal::bit_extract<single_bit_mask>(control_register);
    auto probed_inline = hal::bit_extract<hal::bit_mask{ .position = 15,
                                                          .width = 1 }>(
      control_register);

    // Verify
    expect(that % 0x03A1'0002 == control_register);
    expect(that % 0xA1 == extracted);
    expect(that % 1 == probed);
    expect(that % 0 == probed_inline);
  };
}

void bit_modify_runtime_test()
{
  using namespace boost::ut;

  "hal::bit_modify runtime APIs"_test = []() {
    // Setup
    std::uint32_t volatile control_register = (1U << 15U) | (1U << 16U);
    constexpr auto enable_bit = hal::bit_mask::from(1);
    constexpr auto high_power_mode = hal::bit_mask::from(15);
    constexpr auto clock_divider = hal::bit_mask::from(20, 23);
    constexpr auto extractor_mask = hal::bit_mask::from(16, 23);
    constexpr auto single_bit_mask = hal::bit_mask::from(1);

    // Exercise
    hal::bit_modify(control_register)
      .set(enable_bit)
      .clear(high_power_mode)
      .insert(clock_divider, 0xAU);
    auto extracted = hal::bit_extract(extractor_mask, control_register);
    auto probed = hal::bit_extract(single_bit_mask, control_register);
    auto probed_inline = hal::bit_extract(
      hal::bit_mask{ .position = 15, .width = 1 }, control_register);

    // Verify
    expect(that % 0x00A1'0002 == control_register);
    expect(that % 0xA1 == extracted);
    expect(that % 1 == probed);
    expect(that % 0 == probed_inline);
  };
}

void nibble_byte_mask_test()
{
  using namespace boost::ut;

  "Test nibble_m & byte_m"_test = []() {
    constexpr std::uint32_t expected = 0xAA'55'02'34;
    constexpr std::array<hal::byte, 2> data{ 0x23, 0x40 };
    constexpr std::uint16_t header = 0xAA55;
    auto const value =
      hal::bit_value(0U)
        .insert<hal::byte_m<2, 3>>(header)
        .insert<hal::nibble_m<1, 2>>(data[0])
        .insert<hal::nibble_m<0>>(hal::bit_extract<hal::nibble_m<1>>(data[1]))
        .to<std::uint32_t>();
    expect(that % expected == value)
      << std::hex << expected << "::" << value
      << " :: start = " << hal::nibble_mask<1, 3>::value.width
      << " :: end = " << hal::nibble_mask<1, 3>::value.position;
  };
}

void generate_field_of_ones_test()
{
  using namespace boost::ut;

  "hal::generate_field_of_ones fills the low bits"_test = []() {
    // Exercise
    constexpr auto byte = hal::generate_field_of_ones<8, std::uint32_t>();
    constexpr auto full = hal::generate_field_of_ones<32, std::uint32_t>();
    constexpr auto wide = hal::generate_field_of_ones<40, std::uint64_t>();

    // Verify
    expect(that % 0xFFU == byte);
    expect(that % 0xFFFF'FFFFU == full);
    expect(that % 0xFF'FFFF'FFFFULL == wide);
  };
}

void bit_limits_test()
{
  using namespace boost::ut;

  "hal::bit_limits of a field narrower than its type"_test = []() {
    // Exercise
    constexpr auto signed_max = hal::bit_limits<8, std::int32_t>::max();
    constexpr auto signed_min = hal::bit_limits<8, std::int32_t>::min();
    constexpr auto unsigned_max = hal::bit_limits<12, std::uint16_t>::max();
    constexpr auto unsigned_min = hal::bit_limits<12, std::uint16_t>::min();
    constexpr auto one_bit_max = hal::bit_limits<1, std::int8_t>::max();
    constexpr auto one_bit_min = hal::bit_limits<1, std::int8_t>::min();

    // Verify
    expect(that % std::int32_t{ 127 } == signed_max);
    expect(that % std::int32_t{ -128 } == signed_min);
    expect(that % std::uint16_t{ 4095 } == unsigned_max);
    expect(that % std::uint16_t{ 0 } == unsigned_min);
    expect(that % std::int8_t{ 0 } == one_bit_max);
    expect(that % std::int8_t{ -1 } == one_bit_min);
  };

  "hal::bit_limits of a field as wide as its type"_test = []() {
    // Exercise
    constexpr auto int8_max = hal::bit_limits<8, std::int8_t>::max();
    constexpr auto int8_min = hal::bit_limits<8, std::int8_t>::min();
    constexpr auto int32_max = hal::bit_limits<32, std::int32_t>::max();
    constexpr auto int32_min = hal::bit_limits<32, std::int32_t>::min();
    constexpr auto int64_max = hal::bit_limits<64, std::int64_t>::max();
    constexpr auto int64_min = hal::bit_limits<64, std::int64_t>::min();
    constexpr auto uint64_max = hal::bit_limits<64, std::uint64_t>::max();

    // Verify
    expect(that % std::numeric_limits<std::int8_t>::max() == int8_max);
    expect(that % std::numeric_limits<std::int8_t>::min() == int8_min);
    expect(that % std::numeric_limits<std::int32_t>::max() == int32_max);
    expect(that % std::numeric_limits<std::int32_t>::min() == int32_min);
    expect(that % std::numeric_limits<std::int64_t>::max() == int64_max);
    expect(that % std::numeric_limits<std::int64_t>::min() == int64_min);
    expect(that % std::numeric_limits<std::uint64_t>::max() == uint64_max);
  };

  "hal::bit_limits of a field wider than 32 bits"_test = []() {
    // Exercise
    constexpr auto unsigned_max = hal::bit_limits<40, std::uint64_t>::max();
    constexpr auto signed_max = hal::bit_limits<40, std::int64_t>::max();
    constexpr auto signed_min = hal::bit_limits<40, std::int64_t>::min();

    // Verify
    expect(that % std::uint64_t{ 0xFF'FFFF'FFFF } == unsigned_max);
    expect(that % std::int64_t{ 0x7F'FFFF'FFFF } == signed_max);
    expect(that % std::int64_t{ -0x80'0000'0000 } == signed_min);
  };
}

void bit_is_set_test()
{
  using namespace boost::ut;

  "hal::bit_is_set"_test = []() {
    // Setup
    constexpr auto lowest = hal::bit_mask::from<0>();
    constexpr auto middle = hal::bit_mask::from<15>();
    constexpr auto highest = hal::bit_mask::from<31>();
    std::uint32_t volatile status = (1U << 0U) | (1U << 31U);

    // Exercise
    bool const lowest_set = hal::bit_is_set<lowest>(status);
    bool const middle_set = hal::bit_is_set<middle>(status);
    bool const highest_set = hal::bit_is_set<highest>(status);

    // Verify
    expect(lowest_set);
    expect(not middle_set);
    expect(highest_set);
  };

  "hal::bit_is_set with other widths and at compile time"_test = []() {
    // Setup
    constexpr auto top_of_byte = hal::bit_mask::from<7>();
    constexpr auto top_of_u64 = hal::bit_mask::from<63>();

    // Exercise
    constexpr bool byte_set =
      hal::bit_is_set<top_of_byte>(std::uint8_t{ 0x80 });
    constexpr bool byte_clear =
      hal::bit_is_set<top_of_byte>(std::uint8_t{ 0x7F });
    constexpr bool u64_set =
      hal::bit_is_set<top_of_u64>(std::uint64_t{ 1 } << 63U);

    // Verify
    expect(byte_set);
    expect(not byte_clear);
    expect(u64_set);
  };
}

void bit_mask_or_test()
{
  using namespace boost::ut;

  "hal::bit_mask operator| combines masks that aren't next to each other"_test =
    []() {
      // Setup
      constexpr auto request_completed0 = hal::bit_mask::from<0>();
      constexpr auto request_completed1 = hal::bit_mask::from<8>();
      constexpr auto request_completed2 = hal::bit_mask::from<16>();
      constexpr auto mailbox_empty = hal::bit_mask::from<26, 28>();
      std::uint32_t volatile status_register = 0;

      // Exercise
      constexpr auto requests_completed =
        std::uint32_t{ 0 } | request_completed0 | request_completed1 |
        request_completed2;
      constexpr auto with_field =
        std::uint32_t{ 0 } | mailbox_empty | request_completed0;
      status_register = requests_completed;

      // Verify
      static_assert(
        std::is_same_v<decltype(requests_completed), std::uint32_t const>);
      expect(that % 0x0001'0101U == requests_completed);
      expect(that % 0x1C00'0001U == with_field);
      expect(that % 0x0001'0101U == status_register);
    };

  "hal::bit_mask operator| keeps the integer's width"_test = []() {
    // Setup
    constexpr auto lowest = hal::bit_mask::from<0>();
    constexpr auto top_of_u64 = hal::bit_mask::from<63>();
    constexpr auto top_of_byte = hal::bit_mask::from<7>();

    // Exercise
    constexpr auto wide = std::uint64_t{ 0 } | top_of_u64 | lowest;
    constexpr auto byte = std::uint8_t{ 0b0000'0010 } | top_of_byte;

    // Verify
    static_assert(std::is_same_v<decltype(wide), std::uint64_t const>);
    static_assert(std::is_same_v<decltype(byte), std::uint8_t const>);
    expect(that % 0x8000'0000'0000'0001U == wide);
    expect(that % 0b1000'0010U == byte);
  };
}

void bit_value_assign_test()
{
  using namespace boost::ut;

  "hal::bit_value operator= sets the value and returns the instance"_test =
    []() {
      // Setup
      hal::bit_value<std::uint32_t> value;

      // Exercise
      auto& result = (value = 0x4U);
      result.set<hal::bit_mask::from<0>()>();

      // Verify
      expect(&result == &value);
      expect(that % 0x5U == value.get());
    };
}

/// The bit operations written the long way, to check the cheap ones against
template<std::unsigned_integral T>
struct reference_bits
{
  static constexpr T clear_lowest_set_bit(T p_value)
  {
    if (p_value == 0U) {
      return p_value;
    }
    auto const position = static_cast<std::uint32_t>(std::countr_zero(p_value));
    return hal::bit_value<T>(p_value)
      .clear(hal::bit_mask::from(position))
      .get();
  }

  static constexpr T clear_highest_set_bit(T p_value)
  {
    if (p_value == 0U) {
      return p_value;
    }
    auto const position = static_cast<std::uint32_t>(
      (std::numeric_limits<T>::digits - 1) - std::countl_zero(p_value));
    return hal::bit_value<T>(p_value)
      .clear(hal::bit_mask::from(position))
      .get();
  }
};

void lowest_and_highest_bit_test()
{
  using namespace boost::ut;

  "hal::bit_value::clear_lowest_set_bit"_test = []() {
    // Setup
    hal::bit_value<std::uint32_t> middle = 0b1011'0100U;
    hal::bit_value<std::uint32_t> top_only = 0x8000'0000U;
    hal::bit_value<std::uint32_t> zero = 0U;

    // Exercise
    middle.clear_lowest_set_bit();
    top_only.clear_lowest_set_bit();
    zero.clear_lowest_set_bit();

    // Verify
    expect(that % 0b1011'0000U == middle.get());
    expect(that % 0U == top_only.get());
    expect(that % 0U == zero.get());
  };

  "hal::bit_value::set_lowest_clear_bit"_test = []() {
    // Setup
    hal::bit_value<std::uint32_t> middle = 0b1011'0111U;
    hal::bit_value<std::uint32_t> zero = 0U;
    hal::bit_value<std::uint32_t> all_ones = 0xFFFF'FFFFU;

    // Exercise
    middle.set_lowest_clear_bit();
    zero.set_lowest_clear_bit();
    all_ones.set_lowest_clear_bit();

    // Verify
    expect(that % 0b1011'1111U == middle.get());
    expect(that % 0b1U == zero.get());
    expect(that % 0xFFFF'FFFFU == all_ones.get());
  };

  "hal::bit_value::clear_highest_set_bit"_test = []() {
    // Setup
    hal::bit_value<std::uint32_t> middle = 0b0010'1100U;
    hal::bit_value<std::uint32_t> all_ones = 0xFFFF'FFFFU;
    hal::bit_value<std::uint32_t> zero = 0U;

    // Exercise
    middle.clear_highest_set_bit();
    all_ones.clear_highest_set_bit();
    zero.clear_highest_set_bit();

    // Verify
    expect(that % 0b0000'1100U == middle.get());
    expect(that % 0x7FFF'FFFFU == all_ones.get());
    expect(that % 0U == zero.get());
  };

  "hal::bit_value::set_highest_clear_bit"_test = []() {
    // Setup
    hal::bit_value<std::uint8_t> middle = 0b1101'0011U;
    hal::bit_value<std::uint32_t> zero = 0U;
    hal::bit_value<std::uint32_t> all_ones = 0xFFFF'FFFFU;

    // Exercise
    middle.set_highest_clear_bit();
    zero.set_highest_clear_bit();
    all_ones.set_highest_clear_bit();

    // Verify
    expect(that % 0b1111'0011U == middle.get());
    expect(that % 0x8000'0000U == zero.get());
    expect(that % 0xFFFF'FFFFU == all_ones.get());
  };

  "lowest and highest bit operations chain, and run at compile time"_test =
    []() {
      // Setup
      constexpr auto chained = []() {
        hal::bit_value<std::uint64_t> value = 0U;
        value.set_highest_clear_bit()
          .set_lowest_clear_bit()
          .set_lowest_clear_bit()
          .clear_lowest_set_bit();
        return value.get();
      };

      // Exercise
      constexpr std::uint64_t result = chained();

      // Verify
      expect(that % ((std::uint64_t{ 1 } << 63U) | 0b10U) == result);
    };

  "lowest and highest bit operations through hal::bit_modify"_test = []() {
    // Setup
    std::uint32_t volatile reg = 0b0110U;

    // Exercise
    hal::bit_modify(reg).clear_lowest_set_bit().set_highest_clear_bit();

    // Verify
    expect(that % (0x8000'0000U | 0b0100U) == reg);
  };

  "lowest and highest bit operations match the long way for every u8"_test =
    []() {
      using reference = reference_bits<std::uint8_t>;
      for (unsigned raw = 0; raw <= 0xFFU; raw++) {
        // Setup
        auto const value = static_cast<std::uint8_t>(raw);
        auto const inverted = static_cast<std::uint8_t>(~value);

        // Exercise
        // Held as unsigned so that a failure prints a number, not a char
        unsigned const clear_lowest =
          hal::bit_value(value).clear_lowest_set_bit().get();
        unsigned const set_lowest =
          hal::bit_value(value).set_lowest_clear_bit().get();
        unsigned const clear_highest =
          hal::bit_value(value).clear_highest_set_bit().get();
        unsigned const set_highest =
          hal::bit_value(value).set_highest_clear_bit().get();

        // Verify
        unsigned const expected_clear_lowest =
          reference::clear_lowest_set_bit(value);
        unsigned const expected_clear_highest =
          reference::clear_highest_set_bit(value);
        // Setting the lowest 0 is clearing the lowest 1 of the inverse
        unsigned const expected_set_lowest =
          static_cast<std::uint8_t>(~reference::clear_lowest_set_bit(inverted));
        unsigned const expected_set_highest = static_cast<std::uint8_t>(
          ~reference::clear_highest_set_bit(inverted));
        expect(that % expected_clear_lowest == clear_lowest)
          << "value: " << raw;
        expect(that % expected_clear_highest == clear_highest)
          << "value: " << raw;
        expect(that % expected_set_lowest == set_lowest) << "value: " << raw;
        expect(that % expected_set_highest == set_highest) << "value: " << raw;
      }
    };
}

}  // namespace

int main()
{
  bit_modify_compile_time_test();
  bit_modify_runtime_test();
  nibble_byte_mask_test();
  generate_field_of_ones_test();
  bit_limits_test();
  bit_is_set_test();
  bit_mask_or_test();
  bit_value_assign_test();
  lowest_and_highest_bit_test();
}
