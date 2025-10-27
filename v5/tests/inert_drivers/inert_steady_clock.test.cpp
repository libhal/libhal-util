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

#include <libhal-util/inert_drivers/inert_steady_clock.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite inert_steady_clock_test = []() {
  using namespace boost::ut;
  "inert_steady_clock"_test = []() {
    // Setup
    constexpr hal::hertz expected_frequency = 0.5f;
    constexpr std::uint64_t start_uptime = 99;
    constexpr std::uint64_t expected_uptime = 100;

    inert_steady_clock test(expected_frequency, start_uptime);

    // Exercise
    auto frequency_result = test.frequency();
    auto uptime_result = test.uptime();

    // Verify
    expect(that % expected_frequency == frequency_result);
    expect(that % expected_uptime == uptime_result);
  };
};
}  // namespace hal
