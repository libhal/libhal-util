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

#include <libhal-util/inert_drivers/inert_pwm.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite inert_pwm_test = []() {
  using namespace boost::ut;
  "inert_pwm"_test = []() {
    // Setup
    inert_pwm test;

    // Exercise
    // Verify
    test.frequency(0.1f);
    test.duty_cycle(0.1f);
  };
};
}  // namespace hal
