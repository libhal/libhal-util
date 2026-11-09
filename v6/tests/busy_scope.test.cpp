// Copyright 2026 Khalil Estell and the libhal contributors
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

#include <atomic>

#include <boost/ut.hpp>

import hal;
import hal.util;

namespace {
void busy_scope_test()
{
  using namespace boost::ut;

  "busy_scope sets the flag while it lives"_test = []() {
    // Setup
    std::atomic_flag busy;
    int const driver = 0;
    bool set_inside = false;

    // Exercise
    {
      hal::busy_scope const scope(busy, &driver);
      set_inside = busy.test();
    }

    // Verify
    expect(set_inside);
    expect(not busy.test());
  };

  "busy_scope throws if the flag is already set, and leaves it set"_test =
    []() {
      // Setup
      std::atomic_flag busy;
      busy.test_and_set();
      int const driver = 0;
      void const* thrown_by = nullptr;

      // Exercise
      try {
        hal::busy_scope const scope(busy, &driver);
      } catch (hal::device_or_resource_busy const& p_error) {
        thrown_by = p_error.instance();
      }

      // Verify
      expect(that % &driver == thrown_by);
      expect(busy.test());
    };

  "busy_scope clears the flag when the operation throws"_test = []() {
    // Setup
    std::atomic_flag busy;
    int const driver = 0;

    // Exercise
    try {
      hal::busy_scope const scope(busy, &driver);
      throw hal::io_error(&driver);
    } catch (hal::io_error const&) {
      // The operation failed. The flag must be clear for the next one.
    }

    // Verify
    expect(not busy.test());
  };
}
}  // namespace

int main()
{
  busy_scope_test();
}
