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

#include <libhal-util/input_pin.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"input_pin_test"> input_pin_test = [] {
  using namespace boost::ut;
  "operator==(input_pin::settings)"_test = []() {
    input_pin::settings a{};
    input_pin::settings b{};

    expect(a == b);
  };

  "operator!=(input_pin::settings)"_test = []() {
    input_pin::settings a{ .resistor = pin_resistor::pull_up };
    input_pin::settings b{ .resistor = pin_resistor::none };

    expect(a != b);
  };
};
}  // namespace hal
