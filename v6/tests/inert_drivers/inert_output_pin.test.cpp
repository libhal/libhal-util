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

#include <libhal-util/inert_drivers/inert_output_pin.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite inert_output_pin_test = []() {
  using namespace boost::ut;
  "inert_output_pin"_test = []() {
    // Setup
    inert_output_pin test(false);

    // Exercise
    test.level(true);
    auto get_level_high_result = test.level();
    test.level(false);
    auto get_level_low_result = test.level();

    // Verify
    expect(that % true == get_level_high_result);
    expect(that % false == get_level_low_result);
  };

  "default_inert_output_pin"_test = []() {
    // Setup
    hal::output_pin& test = hal::default_inert_output_pin();

    // Exercise
    test.level(true);
    auto get_level_high_result = test.level();
    test.level(false);
    auto get_level_low_result = test.level();

    // Verify
    expect(that % true == get_level_high_result);
    expect(that % false == get_level_low_result);
  };
};
}  // namespace hal
