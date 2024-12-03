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

#include <libhal-util/bit.hpp>
#include <libhal/units.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"bit"> bit_test = [] {
  using namespace boost::ut;

  "hal::bit_modify<template> compile time masks APIs"_test = []() {
    // Setup
    std::uint32_t volatile control_register = 1 << 15 | 1 << 16;
    constexpr auto enable_bit = bit_mask::from(1);
    constexpr auto high_power_mode = bit_mask::from(15);
    constexpr auto clock_divider = bit_mask::from(20, 23);
    constexpr auto phase_delay = bit_mask::from(24, 27);
    constexpr auto extractor_mask = bit_mask::from(16, 23);
    constexpr auto single_bit_mask = bit_mask::from(1);

    // Exercise
    bit_modify(control_register)
      .set<enable_bit>()
      .clear<high_power_mode>()
      .insert<clock_divider>(0xAU)
      .insert<phase_delay, 0x3U>();
    auto extracted = bit_extract<extractor_mask>(control_register);
    auto probed = bit_extract<single_bit_mask>(control_register);
    auto probed_inline =
      bit_extract<bit_mask{ .position = 15, .width = 1 }>(control_register);

    // Verify
    expect(that % 0x03A1'0002 == control_register);
    expect(that % 0xA1 == extracted);
    expect(that % 1 == probed);
    expect(that % 0 == probed_inline);
  };

  "hal::bit_modify runtime APIs"_test = []() {
    // Setup
    std::uint32_t volatile control_register = 1 << 15 | 1 << 16;
    constexpr auto enable_bit = bit_mask::from(1);
    constexpr auto high_power_mode = bit_mask::from(15);
    constexpr auto clock_divider = bit_mask::from(20, 23);
    constexpr auto extractor_mask = bit_mask::from(16, 23);
    constexpr auto single_bit_mask = bit_mask::from(1);

    // Exercise
    bit_modify(control_register)
      .set(enable_bit)
      .clear(high_power_mode)
      .insert(clock_divider, 0xAU);
    auto extracted = bit_extract(extractor_mask, control_register);
    auto probed = bit_extract(single_bit_mask, control_register);
    auto probed_inline =
      bit_extract(bit_mask{ .position = 15, .width = 1 }, control_register);

    // Verify
    expect(that % 0x00A1'0002 == control_register);
    expect(that % 0xA1 == extracted);
    expect(that % 1 == probed);
    expect(that % 0 == probed_inline);
  };

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
};
}  // namespace hal
