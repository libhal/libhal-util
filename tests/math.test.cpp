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

#include <libhal-util/math.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"math_test"> math_test = [] {
  using namespace boost::ut;

  "hal::multiply()"_test = []() {
    "Zero"_test = []() { expect(that % 0 == multiply(0, 0).value()); };
    "One"_test = []() { expect(that % 1 == multiply(1, 1).value()); };
    "Boundaries"_test = []() {
      expect(that % 4294967295UL == multiply(1UL, 4294967295UL).value());
      expect(that % -2147483647L == multiply(-1LL, 2147483647LL).value());
      expect(that % 2147483648L == multiply(-1LL, -2147483648LL).value());
    };
    "Exceptions"_test = []() {
      expect(!multiply(u32{ 5U }, u32{ 4294967295U }));
      expect(!multiply(u32{ 4L }, u32{ 1073741824L }));
    };
    "Standard Usage"_test = []() {
      expect(that % 75 == multiply(15, 5).value());
      expect(that % -100 == multiply(-10, 10).value());
      expect(that % -4 == multiply(2, -2).value());
      expect(that % -1016379 == multiply(-17, 59787).value());
    };
  };

  "hal::distance()"_test = []() {
    "Zero"_test = []() { expect(that % 0 == distance(0, 0)); };
    "One"_test = []() { expect(that % 1 == distance(0, 1)); };
    "Boundaries"_test = []() {
      expect(that % u32{ 4294967294UL } ==
             distance(u32{ 1UL }, std::numeric_limits<u32>::max()));
      expect(that % u32{ 4294967295UL } ==
             distance(std::numeric_limits<std::int32_t>::min(),
                      std::numeric_limits<std::int32_t>::max()));
    };
    "Standard Usage"_test = []() {
      expect(that % 10 == distance(15, 5));
      expect(that % 20 == distance(-10, 10));
      expect(that % 4 == distance(2, -2));
      expect(that % 59804 == distance(-17, 59787));
      expect(that % 221200 == distance(222323, 1123));
    };
  };
  "hal::equal()"_test = []() {
    "zero"_test = []() { expect(equals(0.0f, (0.1f - 0.1f), 0.000001f)); };
    "one"_test = []() { expect(equals(1.0f, (0.5f + 0.5f), 0.000001f)); };
    "boundaries"_test = []() {
      expect(equals(std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    0.000001f));
    };
    "standard"_test = []() {
      expect(equals(0.3f, (0.15f + 0.15f), 0.000001f));
    };
    "standard double"_test = []() {
      expect(equals(0.3, (0.15 + 0.15), 0.000001f));
    };
    "standard default epsilon"_test = []() {
      expect(equals(0.3f, (0.15f + 0.15f)));
    };
    "standard default epsilon not equal"_test = []() {
      expect(false == equals(0.3f, 0.4f));
    };
    "standard not equal"_test = []() {
      expect(false == equals(0.3f, 0.4f, 0.000001f));
    };
    "standard not equal"_test = []() {
      expect(false == equals(0.3001f, 0.3002f, 0.000001f));
    };
  };

  "hal::upscale()"_test = []() {
    "1-bit to 8-bit"_test = []() {
      expect(that % 0x00 == upscale<u8, 1>(0b0));
      expect(that % 0xFF == upscale<u8, 1>(0b1));
    };
    "2-bit to 8-bit"_test = []() {
      expect(that % 0b00000000 == upscale<u8, 2>(0b00));
      expect(that % 0b01010101 == upscale<u8, 2>(0b01));
      expect(that % 0b10101010 == upscale<u8, 2>(0b10));
      expect(that % 0b11111111 == upscale<u8, 2>(0b11));
    };
    "3-bit to 8-bit"_test = []() {
      expect(that % 0b00000000 == upscale<u8, 3>(0b000));
      expect(that % 0b00100100 == upscale<u8, 3>(0b001));
      expect(that % 0b11011011 == upscale<u8, 3>(0b110));
      expect(that % 0b11111111 == upscale<u8, 3>(0b111));
    };
    "4-bit to 8-bit"_test = []() {
      expect(that % 0x00 == upscale<u8, 4>(0x0));
      expect(that % 0x11 == upscale<u8, 4>(0x1));
      expect(that % 0x77 == upscale<u8, 4>(0x7));
      expect(that % 0xAA == upscale<u8, 4>(0xA));
      expect(that % 0xFF == upscale<u8, 4>(0xF));
    };
    "8-bit to 8-bit (identity)"_test = []() {
      expect(that % 0x00 == upscale<u8, 8>(0x00));
      expect(that % 0x42 == upscale<u8, 8>(0x42));
      expect(that % 0xFF == upscale<u8, 8>(0xFF));
    };
    "1-bit to 16-bit"_test = []() {
      expect(that % 0x0000 == upscale<u16, 1>(0b0));
      expect(that % 0xFFFF == upscale<u16, 1>(0b1));
    };
    "4-bit to 16-bit"_test = []() {
      expect(that % 0x0000 == upscale<u16, 4>(0x0));
      expect(that % 0x1111 == upscale<u16, 4>(0x1));
      expect(that % 0x8888 == upscale<u16, 4>(0x8));
      expect(that % 0xFFFF == upscale<u16, 4>(0xF));
    };
    "8-bit to 16-bit"_test = []() {
      expect(that % 0x0000 == upscale<u16, 8>(0x00));
      expect(that % 0x4242 == upscale<u16, 8>(0x42));
      expect(that % 0xFFFF == upscale<u16, 8>(0xFF));
    };
    "12-bit to 16-bit"_test = []() {
      expect(that % 0x0000 == upscale<u16, 12>(0x000));
      expect(that % 0x8008 == upscale<u16, 12>(0x800));
      expect(that % 0xABCA == upscale<u16, 12>(0xABC));
      expect(that % 0xFFFF == upscale<u16, 12>(0xFFF));
    };
    "16-bit to 16-bit (identity)"_test = []() {
      expect(that % 0x0000 == upscale<u16, 16>(0x0000));
      expect(that % 0x1234 == upscale<u16, 16>(0x1234));
      expect(that % 0xFFFF == upscale<u16, 16>(0xFFFF));
    };
    "1-bit to 32-bit"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 1>(0b0));
      expect(that % 0xFFFFFFFFU == upscale<u32, 1>(0b1));
    };
    "8-bit to 32-bit"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 8>(0x00));
      expect(that % 0x42424242U == upscale<u32, 8>(0x42));
      expect(that % 0xFFFFFFFFU == upscale<u32, 8>(0xFF));
    };
    "10-bit to 32-bit"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 10>(0x000));
      expect(that % 0x55555555U == upscale<u32, 10>(0x555));
      expect(that % 0xFFFFFFFFU == upscale<u32, 10>(0x3FF));
    };
    "12-bit to 32-bit"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 12>(0x000));
      expect(that % 0xABCABCABU == upscale<u32, 12>(0xABC));
      expect(that % 0xFFFFFFFFU == upscale<u32, 12>(0xFFF));
    };
    "16-bit to 32-bit"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 16>(0x0000));
      expect(that % 0x12341234U == upscale<u32, 16>(0x1234));
      expect(that % 0xFFFFFFFFU == upscale<u32, 16>(0xFFFF));
    };
    "24-bit to 32-bit"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 24>(0x000000));
      expect(that % 0xABCDEFABU == upscale<u32, 24>(0xABCDEF));
      expect(that % 0xFFFFFFFFU == upscale<u32, 24>(0xFFFFFF));
    };
    "32-bit to 32-bit (identity)"_test = []() {
      expect(that % 0x00000000U == upscale<u32, 32>(0x00000000U));
      expect(that % 0x12345678U == upscale<u32, 32>(0x12345678U));
      expect(that % 0xFFFFFFFFU == upscale<u32, 32>(0xFFFFFFFFU));
    };
    "masking with extra bits"_test = []() {
      // Test that input values with bits beyond bit_width are masked correctly
      expect(that % 0xFF == upscale<u8, 4>(0xFF));  // 0xFF masked to 0xF
      expect(that % 0xFFFF ==
             upscale<u16, 8>(0xFFFF));  // 0xFFFF masked to 0xFF
    };
  };
};
}  // namespace hal
