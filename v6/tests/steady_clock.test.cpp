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

#include <chrono>
#include <cstdint>
#include <limits>

#include <boost/ut.hpp>

import hal;
import hal.util;

namespace {

using namespace mp_units::si::unit_symbols;
using namespace std::chrono_literals;

constexpr auto expected_frequency = 750 * MHz;

// TODO(kammce): replace with `import hal.util.mock:steady_clock;` once
// hal.util.mock is migrated (see migration plan batch 5).
class dummy_steady_clock : public hal::steady_clock
{
public:
  std::uint64_t m_uptime = 0;

private:
  hal::hertz driver_frequency() override
  {
    return expected_frequency;
  }

  hal::u64 driver_uptime() override
  {
    return m_uptime++;
  }
};

void deadline_ticks_test()
{
  using namespace boost::ut;

  "deadline_ticks rounds up and adds the tick in progress"_test = []() {
    // Exercise
    // 1 ms at 750 MHz is 750,000 ticks.
    auto const ticks = hal::deadline_ticks(750 * MHz, 1ms);
    // 25 ms at 72 MHz is 1,800,000 ticks.
    auto const cpu_ticks = hal::deadline_ticks(72 * MHz, 25ms);
    // 1 ns at 8 MHz is 0.008 of a tick.
    auto const tiny_ticks = hal::deadline_ticks(8 * MHz, 1ns);
    // 1.5 s at 3 Hz is 4.5 ticks, which counts the whole seconds and the rest
    // of a second separately.
    auto const split_ticks = hal::deadline_ticks(3 * Hz, 1'500ms);

    // Verify
    expect(that % 750'001 == ticks);
    expect(that % 1'800'001 == cpu_ticks);
    expect(that % 2 == tiny_ticks);
    expect(that % 6 == split_ticks);
  };

  "deadline_ticks works for a clock faster than 1 GHz"_test = []() {
    // Exercise
    // A tick of a 2 GHz clock is shorter than a nanosecond.
    auto const ticks = hal::deadline_ticks(2'000 * MHz, 1ms);

    // Verify
    expect(that % 2'000'001 == ticks);
  };

  "deadline_ticks is the next tick for a duration that isn't positive"_test =
    []() {
      // Exercise
      auto const zero = hal::deadline_ticks(750 * MHz, 0ms);
      auto const negative = hal::deadline_ticks(750 * MHz, -1ms);

      // Verify
      expect(that % 1 == zero);
      expect(that % 1 == negative);
    };

  "deadline_ticks counts up to 2^32 seconds without overflowing"_test = []() {
    // Setup
    constexpr auto max_u32 = std::numeric_limits<hal::u32>::max();
    constexpr auto max_u64 = std::numeric_limits<hal::u64>::max();
    constexpr auto max_seconds = std::chrono::seconds(max_u32);

    // Exercise
    // The largest clock for the most whole seconds that are counted, with the
    // rest of a second on top, is the largest count that doesn't saturate.
    auto const largest_count =
      hal::deadline_ticks(max_u32 * Hz, max_seconds + 999'999'999ns);
    auto const below_seconds_limit = hal::deadline_ticks(1 * Hz, max_seconds);
    // 2^32 seconds or more gives the most ticks.
    auto const seconds_limit =
      hal::deadline_ticks(1 * Hz, max_seconds + std::chrono::seconds(1));
    auto const largest =
      hal::deadline_ticks(max_u32 * Hz, hal::time_duration::max());

    // Verify
    // (2^32 - 1)^2 ticks for the seconds, 4,294,967,291 for the rest, and the
    // tick in progress.
    expect(that % 18'446'744'069'414'584'317ULL == largest_count);
    expect(that % 4'294'967'296ULL == below_seconds_limit);
    expect(that % max_u64 == seconds_limit);
    expect(that % max_u64 == largest);
  };
}

void future_deadline_test()
{
  using namespace boost::ut;

  "hal::future_deadline(hal::steady_clock, relative_time)"_test = []() {
    // Setup
    dummy_steady_clock test_steady_clock;
    test_steady_clock.m_uptime = 1'000;

    // Exercise
    auto const deadline = hal::future_deadline(test_steady_clock, 1ms);

    // Verify
    // 1 ms at 750 MHz is 750,000 ticks, plus the tick in progress.
    expect(that % 751'001 == deadline);
  };

  "hal::future_deadline is the next tick for a duration that isn't positive"_test =
    []() {
      // Setup
      dummy_steady_clock test_steady_clock;
      test_steady_clock.m_uptime = 5;

      // Exercise
      auto const deadline = hal::future_deadline(test_steady_clock, 0ms);

      // Verify
      expect(that % 6 == deadline);
    };

  "hal::future_deadline saturates past what 64 bits can count"_test = []() {
    // Setup
    constexpr auto max_u64 = std::numeric_limits<hal::u64>::max();
    dummy_steady_clock test_steady_clock;
    test_steady_clock.m_uptime = max_u64 - 10;

    // Exercise
    auto const deadline = hal::future_deadline(test_steady_clock, 1ms);

    // Verify
    expect(that % max_u64 == deadline);
  };
}
}  // namespace

int main()
{
  deadline_ticks_test();
  future_deadline_test();
}
