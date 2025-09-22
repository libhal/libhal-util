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

#include <libhal-util/bit_bang_i2c.hpp>

#include <numeric>

#include <libhal-util/mock/input_pin.hpp>
#include <libhal-util/mock/output_pin.hpp>
#include <libhal-util/mock/steady_clock.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"bit_bang_i2c"> bit_bang_i2c_test = [] {
  using namespace boost::ut;
  using namespace hal::literals;

  "ctor"_test = []() {
    hal::mock_output_pin clock;
    hal::mock_output_pin data;
    hal::bit_bang_i2c::pins pins{
      .sda = &data,
      .scl = &clock,
    };
    hal::mock_steady_clock steady_clock;

    steady_clock.set_frequency(1.0_MHz);
    std::vector<hal::u64> uptime_set(4098);
    // NOTE: We disabled clang-tidy linting for this line because
    // std::ranges::iota doesn't appear to work in clang-17.
    // NOLINTNEXTLINE(modernize-use-ranges)
    std::iota(uptime_set.begin(), uptime_set.end(), 0);
    std::queue<hal::u64> uptimes(uptime_set.begin(), uptime_set.end());
    steady_clock.set_uptimes(uptimes);

    hal::bit_bang_i2c test_subject(pins, steady_clock);
  };
};
}  // namespace hal
