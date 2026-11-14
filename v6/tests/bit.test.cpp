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

#include <cstdint>
#include <ios>
#include <limits>

#include <boost/ut.hpp>

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
}  // namespace

int main()
{
  bit_modify_compile_time_test();
  bit_modify_runtime_test();
  nibble_byte_mask_test();
  generate_field_of_ones_test();
  bit_limits_test();
  bit_is_set_test();
}
