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

#include <boost/ut.hpp>

import hal;
import hal.util;

namespace {

using namespace mp_units::si::unit_symbols;

constexpr auto expected_frequency = 750 * MHz;

// TODO(kammce): replace with `import hal.util.mock:steady_clock;` once
// hal.util.mock is migrated (see migration plan batch 5).
class dummy_steady_clock : public hal::steady_clock
{
public:
  std::uint64_t m_uptime = 0;

private:
  async::future<hal::hertz> driver_frequency(async::context&) override
  {
    return expected_frequency;
  }

  async::future<hal::u64> driver_uptime(async::context&) override
  {
    return m_uptime++;
  }
};

async::inplace_context<1024> ctx;

void future_deadline_test()
{
  using namespace boost::ut;

  "hal::future_deadline(hal::steady_clock, relative_time)"_test = []() {
    // Setup
    using namespace std::chrono_literals;

    dummy_steady_clock test_steady_clock;
    static constexpr auto expected = 1'000'000;  // 1ms in nanoseconds
    test_steady_clock.m_uptime = 0;

    // Exercise
    auto deadline = hal::future_deadline(ctx, test_steady_clock, 1ms);

    while (not deadline.done()) {
      deadline.resume();  // should finish
    }

    // Verify
    expect(that % expected == deadline.value());
  };
}

}  // namespace

int main()
{
  future_deadline_test();
}
