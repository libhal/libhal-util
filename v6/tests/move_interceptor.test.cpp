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

#include <libhal-util/move_interceptor.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"move_interceptor_test"> move_interceptor_test = [] {
  using namespace boost::ut;

  "move_interceptor<mock>::intercept"_test = []() {
    struct mock : public move_interceptor<mock>
    {
      mock() = default;
      mock(mock&) noexcept = delete;
      mock& operator=(mock&) noexcept = delete;
      mock& operator=(mock&&) noexcept = default;
      mock(mock&&) noexcept = default;

      void intercept(mock* p_old_self)
      {
        p_old_self->count++;
      }

      int count = 0;
    };

    // Setup
    mock mock1;

    // Exercise
    auto mock2 = std::move(mock1);
    // Verify
    expect(that % 1 == mock2.count);

    // Exercise
    auto mock3 = std::move(mock2);
    // Verify
    expect(that % 2 == mock3.count);

    // Exercise
    auto mock4 = std::move(mock3);
    // Verify
    expect(that % 3 == mock4.count);

    // Exercise
    auto mock5 = std::move(mock4);
    // Verify
    expect(that % 4 == mock5.count);
  };
};
}  // namespace hal
