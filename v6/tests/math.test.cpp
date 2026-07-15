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

}  // namespace

int main()
{
  multiply_test();
  distance_test();
  equals_test();
}
