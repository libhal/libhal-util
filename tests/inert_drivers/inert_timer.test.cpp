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

#include <libhal-util/inert_drivers/inert_timer.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite inert_timer_test = []() {
  using namespace boost::ut;
  "inert_timer"_test = []() {
    // Setup
    hal::callback<void(void)> const callback = []() {};
    hal::time_duration const delay = {};
    auto is_running = true;
    auto is_not_running = false;
    inert_timer test1(is_running);
    inert_timer test2(is_not_running);

    // Exercise
    auto is_running_result1 = test1.is_running();
    auto is_running_result2 = test2.is_running();
    test1.cancel();
    test1.schedule(callback, delay);

    // Verify
    expect(that % true == is_running_result1);
    expect(that % false == is_running_result2);
  };
};
}  // namespace hal
