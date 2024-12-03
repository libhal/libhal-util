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

#include <libhal-util/map.hpp>

#include <limits>

#include <libhal-util/math.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"map"> map_test = [] {
  using namespace boost::ut;

  "hal::map<std::floating_point>()"_test = []() {
    "Zero"_test = []() {
      expect(that % 0.0_d ==
             map(0.0, std::make_pair(0.0, 10.0), std::make_pair(0.0, 100.0)));
    };
    "Boundaries"_test = []() {
      constexpr auto min = double{ std::numeric_limits<std::int32_t>::min() };
      constexpr auto max = double{ std::numeric_limits<std::int32_t>::max() };

      expect(that % 0.0_d ==
             map(0.0, std::make_pair(min, max), std::make_pair(min, max)));

      expect(that % 0.5_d ==
             map(0.0, std::make_pair(min, max), std::make_pair(0.0, 1.0)));

      expect(equals(
        max,
        map(1338.0, std::make_pair(1337.0, 1338.0), std::make_pair(min, max))));

      expect(equals(
        min,
        map(1337.0, std::make_pair(1337.0, 1338.0), std::make_pair(min, max))));
    };

    "Standard usage"_test = []() {
      expect(that % 50.0 ==
             map(5.0, std::make_pair(0.0, 10.0), std::make_pair(0.0, 100.0)));
      expect(that % 0.0 == map(5.0,
                               std::make_pair(0.0, 10.0),
                               std::make_pair(-100.0, 100.0)));
      expect(that % 50.0 ==
             map(-5.0, std::make_pair(-10.0, 0.0), std::make_pair(0.0, 100.0)));
      expect(that % 25.0 == map(-75.0,
                                std::make_pair(-100.0, 0.0),
                                std::make_pair(0.0, 100.0)));
      expect(that % -175.0 == map(-75.0,
                                  std::make_pair(-100.0, 0.0),
                                  std::make_pair(-200.0, -100.0)));
      expect(that % 10.0 ==
             map(0.0, std::make_pair(-10.0, 10.0), std::make_pair(10.0, 10.0)));
      expect(that % 0.0 ==
             map(0.0, std::make_pair(-1.0, 1.0), std::make_pair(-1.0, 1.0)));
    };
  };
};
}  // namespace hal
