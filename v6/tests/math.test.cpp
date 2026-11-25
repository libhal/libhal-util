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

#include <limits>

#include <boost/ut.hpp>

import hal;
import hal.util;

namespace {

void multiply_test()
{
  using namespace boost::ut;

  "hal::multiply()"_test = [] {
    "Zero"_test = [] { expect(that % 0 == hal::multiply(0, 0).value()); };
    "One"_test = [] { expect(that % 1 == hal::multiply(1, 1).value()); };
    "Boundaries"_test = [] {
      expect(that % 4294967295UL == hal::multiply(1UL, 4294967295UL).value());
      expect(that % -2147483647L ==
             hal::multiply(-1LL, 2147483647LL).value());
      expect(that % 2147483648L ==
             hal::multiply(-1LL, -2147483648LL).value());
    };
    "Exceptions"_test = [] {
      expect(!hal::multiply(hal::u32{ 5U }, hal::u32{ 4294967295U }));
      expect(!hal::multiply(hal::u32{ 4L }, hal::u32{ 1073741824L }));
    };
    "Standard Usage"_test = [] {
      expect(that % 75 == hal::multiply(15, 5).value());
      expect(that % -100 == hal::multiply(-10, 10).value());
      expect(that % -4 == hal::multiply(2, -2).value());
      expect(that % -1016379 == hal::multiply(-17, 59787).value());
    };
  };
}

void distance_test()
{
  using namespace boost::ut;

  "hal::distance()"_test = [] {
    "Zero"_test = [] { expect(that % 0 == hal::distance(0, 0)); };
    "One"_test = [] { expect(that % 1 == hal::distance(0, 1)); };
    "Boundaries"_test = [] {
      expect(that % hal::u32{ 4294967294UL } ==
             hal::distance(hal::u32{ 1UL },
                           std::numeric_limits<hal::u32>::max()));
      expect(that % hal::u32{ 4294967295UL } ==
             hal::distance(std::numeric_limits<std::int32_t>::min(),
                           std::numeric_limits<std::int32_t>::max()));
    };
    "Standard Usage"_test = [] {
      expect(that % 10 == hal::distance(15, 5));
      expect(that % 20 == hal::distance(-10, 10));
      expect(that % 4 == hal::distance(2, -2));
      expect(that % 59804 == hal::distance(-17, 59787));
      expect(that % 221200 == hal::distance(222323, 1123));
    };
  };
}

void equals_test()
{
  using namespace boost::ut;

  "hal::equals()"_test = [] {
    "zero"_test = [] {
      expect(hal::equals(0.0f, (0.1f - 0.1f), 0.000001f));
    };
    "one"_test = [] { expect(hal::equals(1.0f, (0.5f + 0.5f), 0.000001f)); };
    "boundaries"_test = [] {
      expect(hal::equals(std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         0.000001f));
    };
    "standard"_test = [] {
      expect(hal::equals(0.3f, (0.15f + 0.15f), 0.000001f));
    };
    "standard double"_test = [] {
      expect(hal::equals(0.3, (0.15 + 0.15), 0.000001f));
    };
    "standard default epsilon"_test = [] {
      expect(hal::equals(0.3f, (0.15f + 0.15f)));
    };
    "standard default epsilon not equal"_test = [] {
      expect(false == hal::equals(0.3f, 0.4f));
    };
    "standard not equal"_test = [] {
      expect(false == hal::equals(0.3f, 0.4f, 0.000001f));
    };
    "standard not equal"_test = [] {
      expect(false == hal::equals(0.3001f, 0.3002f, 0.000001f));
    };
  };
}

void divide_test()
{
  using namespace boost::ut;
  using hal::rounding;

  "hal::divide()"_test = [] {
    "Exact division doesn't round"_test = [] {
      expect(that % 4U ==
             hal::divide<rounding::floor, hal::u32>({ .num = 12U, .den = 3U }));
      expect(that % 4U ==
             hal::divide<rounding::ceil, hal::u32>({ .num = 12U, .den = 3U }));
      expect(that % 4U ==
             hal::divide<rounding::round, hal::u32>({ .num = 12U, .den = 3U }));
      expect(that % -4 ==
             hal::divide<rounding::ceil, hal::i32>({ .num = 12, .den = -3 }));
      expect(that % 0 ==
             hal::divide<rounding::floor, hal::i32>({ .num = 0, .den = -3 }));
    };

    "floor rounds toward negative infinity"_test = [] {
      expect(that % 3 ==
             hal::divide<rounding::floor, hal::i32>({ .num = 7, .den = 2 }));
      expect(that % -4 ==
             hal::divide<rounding::floor, hal::i32>({ .num = -7, .den = 2 }));
      expect(that % -4 ==
             hal::divide<rounding::floor, hal::i32>({ .num = 7, .den = -2 }));
      expect(that % 3 ==
             hal::divide<rounding::floor, hal::i32>({ .num = -7, .den = -2 }));
      expect(that % 0U ==
             hal::divide<rounding::floor, hal::u32>({ .num = 3U, .den = 4U }));
    };

    "ceil rounds toward positive infinity"_test = [] {
      expect(that % 4 ==
             hal::divide<rounding::ceil, hal::i32>({ .num = 7, .den = 2 }));
      expect(that % -3 ==
             hal::divide<rounding::ceil, hal::i32>({ .num = -7, .den = 2 }));
      expect(that % -3 ==
             hal::divide<rounding::ceil, hal::i32>({ .num = 7, .den = -2 }));
      expect(that % 4 ==
             hal::divide<rounding::ceil, hal::i32>({ .num = -7, .den = -2 }));
      expect(that % 4U ==
             hal::divide<rounding::ceil, hal::u32>({ .num = 10U, .den = 3U }));
      expect(that % 1U ==
             hal::divide<rounding::ceil, hal::u32>({ .num = 1U, .den = 7U }));
    };

    "round goes to the nearest integer"_test = [] {
      expect(that % 2 ==
             hal::divide<rounding::round, hal::i32>({ .num = 7, .den = 3 }));
      expect(that % 3 ==
             hal::divide<rounding::round, hal::i32>({ .num = 8, .den = 3 }));
      expect(that % 1 ==
             hal::divide<rounding::round, hal::i32>({ .num = 10, .den = 7 }));
      expect(that % 1 ==
             hal::divide<rounding::round, hal::i32>({ .num = 3, .den = 4 }));
      expect(that % 0 ==
             hal::divide<rounding::round, hal::i32>({ .num = 1, .den = 3 }));
      expect(that % -2 ==
             hal::divide<rounding::round, hal::i32>({ .num = -7, .den = 3 }));
      expect(that % -2 ==
             hal::divide<rounding::round, hal::i32>({ .num = 7, .den = -3 }));
    };

    "round takes a half to the even neighbor"_test = [] {
      expect(that % 0 ==
             hal::divide<rounding::round, hal::i32>({ .num = 1, .den = 2 }));
      expect(that % 2 ==
             hal::divide<rounding::round, hal::i32>({ .num = 3, .den = 2 }));
      expect(that % 2 ==
             hal::divide<rounding::round, hal::i32>({ .num = 5, .den = 2 }));
      expect(that % 4 ==
             hal::divide<rounding::round, hal::i32>({ .num = 7, .den = 2 }));
      expect(that % -2 ==
             hal::divide<rounding::round, hal::i32>({ .num = -5, .den = 2 }));
      expect(that % -2 ==
             hal::divide<rounding::round, hal::i32>({ .num = 5, .den = -2 }));
      expect(that % 2 ==
             hal::divide<rounding::round, hal::i32>({ .num = -5, .den = -2 }));
      expect(that % -4 ==
             hal::divide<rounding::round, hal::i32>({ .num = -7, .den = 2 }));
      expect(that % 2U ==
             hal::divide<rounding::round, hal::u32>({ .num = 10U, .den = 4U }));
      expect(that % 4U ==
             hal::divide<rounding::round, hal::u32>({ .num = 14U, .den = 4U }));
    };

    "Doesn't overflow at the limits"_test = [] {
      constexpr auto max32 = std::numeric_limits<hal::u32>::max();
      constexpr auto max64 = std::numeric_limits<hal::u64>::max();
      constexpr auto min32 = std::numeric_limits<hal::i32>::min();
      constexpr auto max_signed = std::numeric_limits<hal::i32>::max();
      expect(that % 0x8000'0000U == hal::divide<rounding::ceil, hal::u32>(
                                      { .num = max32, .den = 2U }));
      expect(that % 1U == hal::divide<rounding::ceil, hal::u32>(
                            { .num = max32, .den = max32 }));
      expect(that % 0x8000'0000U == hal::divide<rounding::round, hal::u32>(
                                      { .num = max32, .den = 2U }));
      expect(
        that % 0x8000'0000'0000'0000ULL ==
        hal::divide<rounding::ceil, hal::u64>({ .num = max64, .den = 2U }));
      expect(that % 1 == hal::divide<rounding::round, hal::i32>(
                           { .num = min32, .den = min32 }));
      expect(that % min32 == hal::divide<rounding::floor, hal::i32>(
                               { .num = min32, .den = 1 }));
      expect(that % -1 == hal::divide<rounding::round, hal::i32>(
                            { .num = max_signed, .den = min32 }));
      expect(that % 0 == hal::divide<rounding::ceil, hal::i32>(
                           { .num = max_signed, .den = min32 }));
    };

    "ceil of zero is zero"_test = [] {
      expect(that % 0U ==
             hal::divide<rounding::ceil, hal::u32>({ .num = 0U, .den = 3U }));
      expect(that % 0ULL ==
             hal::divide<rounding::ceil, hal::u64>({ .num = 0U, .den = 3U }));
      expect(that % hal::u8{ 0 } ==
             hal::divide<rounding::ceil, hal::u8>({ .num = 0, .den = 3 }));
      static_assert(
        hal::divide<rounding::ceil, hal::u32>({ .num = 0U, .den = 3U }) == 0U);
    };

    "Works for a narrow type"_test = [] {
      expect(that % hal::u8{ 128 } ==
             hal::divide<rounding::ceil, hal::u8>({ .num = 255, .den = 2 }));
    };

    "Takes a narrower operand"_test = [] {
      hal::u64 const cycles = 0x1'0000'0001;
      hal::u32 const max_count = 0x0100'0000;
      expect(that % 257ULL == hal::divide<rounding::ceil, hal::u64>(
                                { .num = cycles, .den = max_count }));
    };

    "A denominator of 0 divides by 1"_test = [] {
      // A variable, so the call isn't a constant expression, where a
      // denominator of 0 is a compile error.
      hal::u32 zero = 0;
      expect(that % 7U ==
             hal::divide<rounding::ceil, hal::u32>({ .num = 7U, .den = zero }));
      hal::i32 signed_zero = 0;
      expect(that % -7 == hal::divide<rounding::floor, hal::i32>(
                            { .num = -7, .den = signed_zero }));
    };

    "Is usable in constant expressions"_test = [] {
      static_assert(
        hal::divide<rounding::ceil, hal::u32>({ .num = 10U, .den = 3U }) == 4U);
      static_assert(
        hal::divide<rounding::floor, hal::i32>({ .num = -7, .den = 2 }) == -4);
      static_assert(
        hal::divide<rounding::round, hal::i32>({ .num = -5, .den = 2 }) == -2);
    };
  };
}

}  // namespace

int main()
{
  multiply_test();
  distance_test();
  equals_test();
  divide_test();
}
