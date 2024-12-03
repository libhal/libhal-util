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

#include <libhal-util/output_pin.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"output_pin_test"> output_pin_test = [] {
  using namespace boost::ut;
  "operator==(output_pin::settings)"_test = []() {
    output_pin::settings a{};
    output_pin::settings b{};

    expect(a == b);
  };

  "operator!=(output_pin::settings)"_test = []() {
    output_pin::settings a{ .open_drain = true };
    output_pin::settings b{ .open_drain = false };

    expect(a != b);
  };
};
}  // namespace hal
