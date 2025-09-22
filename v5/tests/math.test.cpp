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
      expect(!multiply(std::uint32_t{ 5U }, std::uint32_t{ 4294967295U }));
      expect(!multiply(std::uint32_t{ 4L }, std::uint32_t{ 1073741824L }));
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
      expect(that % std::uint32_t{ 4294967294UL } ==
             distance(std::uint32_t{ 1UL },
                      std::numeric_limits<std::uint32_t>::max()));
      expect(that % std::uint32_t{ 4294967295UL } ==
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
};
}  // namespace hal
