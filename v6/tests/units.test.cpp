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
#include <ios>
#include <limits>

// must be above <boost/ut.hpp> to print chrono duration objects
template<typename Rep, typename Period>
std::ostream& operator<<(std::ostream& p_os,
                         std::chrono::duration<Rep, Period> const& p_duration)
{
  return p_os << p_duration.count() << " * (" << Period::num << "/"
              << Period::den << ")s";
}

#include <boost/ut.hpp>

import hal;
import hal.util;

using namespace mp_units;

namespace {

void units_test()
{
  using namespace boost::ut;
  using namespace std::literals;

  "cycles_per"_test = []() {
    expect(that % 4000 == hal::cycles_per(1'000'000 * si::hertz, 4ms));
    expect(that % 1'680'000 ==
           hal::cycles_per(140'000'000 * si::hertz, 12'000us));
    expect(that % 10 == hal::cycles_per(10 * si::hertz, 1s));
    expect(that % 720'000 == hal::cycles_per(48'000'000 * si::hertz, 15ms));
    expect(that % 192'000 == hal::cycles_per(12'000'000 * si::hertz, 16ms));
    expect(that % 960'000'000 == hal::cycles_per(8'000'000 * si::hertz, 2min));
    expect(that % 57'600'000 == hal::cycles_per(32'000 * si::hertz, 30min));
    expect(that % 18'446'744'064'966'997'184ULL ==
           hal::cycles_per(1'000'000'000 * si::hertz, 10min));

    // Result of zero means that the time period is smaller than the
    // frequency's period length
    expect(that % 0 == hal::cycles_per(100'000 * si::hertz, 1us));
    expect(that % 0 == hal::cycles_per(100 * si::hertz, 1ms));
  };

  "polls_for counts polls of at least the given cycles"_test = []() {
    // Exercise
    // 100 ms at 36 MHz is 3,600,000 cycles, and a poll takes at least two.
    auto const polls = hal::polls_for(36'000'000 * si::hertz, 100ms, 2);
    // 4 ms at 1 MHz is 4,000 cycles of one cycle each.
    auto const single_cycle_polls =
      hal::polls_for(1'000'000 * si::hertz, 4ms, 1);

    // Verify
    expect(that % 1'800'000 == polls.value());
    expect(that % 4'000 == single_cycle_polls.value());
  };

  "polls_for rounds up, so the polls are never shorter"_test = []() {
    // Exercise
    // 1 ns at 8 MHz is 0.008 cycles: less than one poll.
    auto const tiny_polls = hal::polls_for(8'000'000 * si::hertz, 1ns, 2);
    // 1,001 ns at 8 MHz is 8.008 cycles: 4.004 polls.
    auto const odd_polls = hal::polls_for(8'000'000 * si::hertz, 1'001ns, 2);
    // 1.5 s at 3 Hz is 4.5 cycles, which counts the whole seconds and the
    // rest of a second separately: 1.5 polls.
    auto const split_polls = hal::polls_for(3 * si::hertz, 1'500ms, 3);

    // Verify
    expect(that % 1 == tiny_polls.value());
    expect(that % 5 == odd_polls.value());
    expect(that % 2 == split_polls.value());
  };

  "polls_for is zero for a duration that isn't positive"_test = []() {
    // Exercise
    auto const zero = hal::polls_for(36'000'000 * si::hertz, 0ms, 2);
    auto const negative = hal::polls_for(36'000'000 * si::hertz, -1ms, 2);

    // Verify
    expect(that % 0 == zero.value());
    expect(that % 0 == negative.value());
  };

  "polls_for has no answer when it can't count the polls"_test = []() {
    // Exercise
    auto const no_clock = hal::polls_for(0 * si::hertz, 100ms, 2);
    auto const no_cycles = hal::polls_for(36'000'000 * si::hertz, 100ms, 0);
    // 4,294,967,295 polls of two cycles at 50 MHz take about 171.8 seconds.
    auto const longest = hal::polls_for(50'000'000 * si::hertz, 171s, 2);
    auto const too_long = hal::polls_for(50'000'000 * si::hertz, 172s, 2);
    // The largest clock and cycle count don't overflow.
    constexpr auto max_u32 = std::numeric_limits<hal::u32>::max();
    auto const largest =
      hal::polls_for(max_u32 * si::hertz, hal::time_duration::max(), max_u32);
    auto const largest_short = hal::polls_for(max_u32 * si::hertz, 1s, max_u32);
    // 2^32 seconds is too long, even when the polls would fit.
    auto const below_seconds_limit =
      hal::polls_for(1 * si::hertz, std::chrono::seconds(max_u32), 2);
    auto const seconds_limit = hal::polls_for(
      1 * si::hertz, std::chrono::seconds(hal::u64{ max_u32 } + 1), 2);

    // Verify
    expect(not no_clock.has_value());
    expect(not no_cycles.has_value());
    expect(that % 4'275'000'000U == longest.value());
    expect(not too_long.has_value());
    expect(not largest.has_value());
    expect(that % 2'147'483'648U == below_seconds_limit.value());
    expect(not seconds_limit.has_value());
    expect(that % 1 == largest_short.value());
  };

  "duration wavelength"_test = []() {
    expect(that % std::numeric_limits<hal::hertz::rep>::max() ==
           hal::wavelength<std::femto>(0 * si::hertz).count());
    // 2 GHz, near the top of the u32 range hal::hertz can represent.
    expect(that % 5'000'000 ==
           hal::wavelength<std::femto>(2'000'000'00 * si::hertz).count());
    expect(that % 1'000 == hal::wavelength<std::milli>(1 * si::hertz).count());
    expect(that % 100'000 ==
           hal::wavelength<std::micro>(10 * si::hertz).count());
    expect(that % 1'000 ==
           hal::wavelength<std::nano>(1'000'000 * si::hertz).count());
    expect(that % 1 ==
           hal::wavelength<std::nano>(1'000'000'000 * si::hertz).count());
  };

  "float wavelength"_test = []() {
    expect(hal::equals(0.0f, hal::wavelength(0 * si::hertz)));
    // 2 GHz, near the top of the u32 range hal::hertz can represent.
    expect(
      hal::equals(0.0000000005f, hal::wavelength(2'000'000'000 * si::hertz)));
    expect(hal::equals(1.0f, hal::wavelength(1 * si::hertz)));
    expect(hal::equals(0.1f, hal::wavelength(10 * si::hertz)));
    expect(hal::equals(0.000001f, hal::wavelength(1'000'000 * si::hertz)));
    expect(
      hal::equals(0.000000001f, hal::wavelength(1'000'000'000 * si::hertz)));
  };

  "duration_from_cycles"_test = []() {
    expect(that % 1'400us ==
           hal::duration_from_cycles(1'000'000 * si::hertz, 1400).value());
    expect(that % 2'380'928ns ==
           hal::duration_from_cycles(14'000'000 * si::hertz, 33333).value());
    expect(that % 10'250ms ==
           std::chrono::duration_cast<std::chrono::milliseconds>(
             hal::duration_from_cycles(1'000 * si::hertz, 10'250).value()));
    expect(
      that % 12'000us ==
      hal::duration_from_cycles(1'000'000'000 * si::hertz, 12'000'000).value());
    expect(that % 0us ==
           hal::duration_from_cycles(1'000'000'000 * si::hertz, 0).value());

    expect(that % 1ns ==
           hal::duration_from_cycles(1'000'000'000 * si::hertz, 1).value());
    expect(that % 1us ==
           hal::duration_from_cycles(1'000'000 * si::hertz, 1).value());
    expect(that % 1ms ==
           hal::duration_from_cycles(1'000 * si::hertz, 1).value());
    expect(that % 1s == hal::duration_from_cycles(1 * si::hertz, 1).value());

    expect(that % 2'000'000us ==
           hal::duration_from_cycles(500'000 * si::hertz, 1'000'000).value());
    expect(that % 1s ==
           hal::duration_from_cycles(1'000'000'000 * si::hertz, 1'000'000'000)
             .value());
    expect(that % 200ms ==
           hal::duration_from_cycles(1'000'000'000 * si::hertz, 200'000'000)
             .value());

    expect(
      that % 25ms ==
      hal::duration_from_cycles(2'000'000'000 * si::hertz, 50'000'000).value());

    // 0 Hz cannot be represented exactly by hal::hertz's u32 rep at this
    // magnitude either way; both branches below still exercise the nullopt
    // (overflow) path since dividing by ~0 produces an out-of-range result.
    expect(!hal::duration_from_cycles(
      0 * si::hertz,
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())));
    expect(!hal::duration_from_cycles(
      0 * si::hertz,
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::min())));
  };
}

}  // namespace

int main()
{
  units_test();
}
