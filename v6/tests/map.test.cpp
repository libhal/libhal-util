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

import hal.util;

namespace {

void map_test()
{
  using namespace boost::ut;

  "hal::map<std::floating_point>()"_test = []() {
    "Zero"_test = []() {
      expect(that % 0.0_d == hal::map(0.0,
                                      std::make_pair(0.0, 10.0),
                                      std::make_pair(0.0, 100.0)));
    };
    "Boundaries"_test = []() {
      constexpr auto min = double{ std::numeric_limits<std::int32_t>::min() };
      constexpr auto max = double{ std::numeric_limits<std::int32_t>::max() };

      expect(that % 0.0_d == hal::map(0.0,
                                      std::make_pair(min, max),
                                      std::make_pair(min, max)));

      expect(that % 0.5_d == hal::map(0.0,
                                      std::make_pair(min, max),
                                      std::make_pair(0.0, 1.0)));

      expect(hal::equals(
        max,
        hal::map(
          1338.0, std::make_pair(1337.0, 1338.0), std::make_pair(min, max))));

      expect(hal::equals(
        min,
        hal::map(
          1337.0, std::make_pair(1337.0, 1338.0), std::make_pair(min, max))));
    };

    "Standard usage"_test = []() {
      expect(that % 50.0 == hal::map(5.0,
                                     std::make_pair(0.0, 10.0),
                                     std::make_pair(0.0, 100.0)));
      expect(that % 0.0 == hal::map(5.0,
                                    std::make_pair(0.0, 10.0),
                                    std::make_pair(-100.0, 100.0)));
      expect(that % 50.0 == hal::map(-5.0,
                                     std::make_pair(-10.0, 0.0),
                                     std::make_pair(0.0, 100.0)));
      expect(that % 25.0 == hal::map(-75.0,
                                     std::make_pair(-100.0, 0.0),
                                     std::make_pair(0.0, 100.0)));
      expect(that % -175.0 == hal::map(-75.0,
                                       std::make_pair(-100.0, 0.0),
                                       std::make_pair(-200.0, -100.0)));
      expect(that % 10.0 == hal::map(0.0,
                                     std::make_pair(-10.0, 10.0),
                                     std::make_pair(10.0, 10.0)));
      expect(that % 0.0 == hal::map(0.0,
                                    std::make_pair(-1.0, 1.0),
                                    std::make_pair(-1.0, 1.0)));
    };
  };
}

}  // namespace

int main()
{
  map_test();
}
