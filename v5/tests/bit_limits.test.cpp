// Copyright 2026 Khalil Estell and the libhal contributors
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
#include <limits>

#include <libhal-util/bit_limits.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"bit_limits_test"> bit_limits_test = [] {
  using namespace boost::ut;

  "bit_limits of a field narrower than its type"_test = []() {
    // Exercise
    constexpr auto signed_max = bit_limits<8, std::int32_t>::max();
    constexpr auto signed_min = bit_limits<8, std::int32_t>::min();
    constexpr auto unsigned_max = bit_limits<12, std::uint16_t>::max();
    constexpr auto unsigned_min = bit_limits<12, std::uint16_t>::min();
    constexpr auto one_bit_max = bit_limits<1, std::int8_t>::max();
    constexpr auto one_bit_min = bit_limits<1, std::int8_t>::min();

    // Verify
    expect(that % std::int32_t{ 127 } == signed_max);
    expect(that % std::int32_t{ -128 } == signed_min);
    expect(that % std::uint16_t{ 4095 } == unsigned_max);
    expect(that % std::uint16_t{ 0 } == unsigned_min);
    expect(that % std::int8_t{ 0 } == one_bit_max);
    expect(that % std::int8_t{ -1 } == one_bit_min);
  };

  "bit_limits of a field as wide as its type"_test = []() {
    // Exercise
    constexpr auto int8_max = bit_limits<8, std::int8_t>::max();
    constexpr auto int8_min = bit_limits<8, std::int8_t>::min();
    constexpr auto int32_max = bit_limits<32, std::int32_t>::max();
    constexpr auto int32_min = bit_limits<32, std::int32_t>::min();
    constexpr auto int64_max = bit_limits<64, std::int64_t>::max();
    constexpr auto int64_min = bit_limits<64, std::int64_t>::min();
    constexpr auto uint64_max = bit_limits<64, std::uint64_t>::max();

    // Verify
    expect(that % std::numeric_limits<std::int8_t>::max() == int8_max);
    expect(that % std::numeric_limits<std::int8_t>::min() == int8_min);
    expect(that % std::numeric_limits<std::int32_t>::max() == int32_max);
    expect(that % std::numeric_limits<std::int32_t>::min() == int32_min);
    expect(that % std::numeric_limits<std::int64_t>::max() == int64_max);
    expect(that % std::numeric_limits<std::int64_t>::min() == int64_min);
    expect(that % std::numeric_limits<std::uint64_t>::max() == uint64_max);
  };

  "bit_limits of a field wider than 32 bits"_test = []() {
    // Exercise
    constexpr auto unsigned_max = bit_limits<40, std::uint64_t>::max();
    constexpr auto signed_max = bit_limits<40, std::int64_t>::max();
    constexpr auto signed_min = bit_limits<40, std::int64_t>::min();

    // Verify
    expect(that % std::uint64_t{ 0xFF'FFFF'FFFF } == unsigned_max);
    expect(that % std::int64_t{ 0x7F'FFFF'FFFF } == signed_max);
    expect(that % std::int64_t{ -0x80'0000'0000 } == signed_min);
  };
};
}  // namespace hal
