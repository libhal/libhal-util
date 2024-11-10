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

#include <libhal-util/inert_drivers/inert_input_pin.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite inert_input_pin_test = []() {
  using namespace boost::ut;
  "inert_input_pin"_test = []() {
    // Setup
    auto level_true = true;
    auto level_false = false;
    auto configure_settings = input_pin::settings{};
    inert_input_pin test1(level_true);
    inert_input_pin test2(level_false);

    // Exercise
    test1.configure(configure_settings);
    auto level_high_result = test1.level();
    auto level_low_result = test2.level();

    // Verify
    expect(that % true == level_high_result);
    expect(that % false == level_low_result);
  };
};
}  // namespace hal
