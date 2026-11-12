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
    expect(that % 600'000'000'000ULL ==
           hal::cycles_per(1'000'000'000 * si::hertz, 10min));

    // A duration shorter than the frequency's period still takes a whole
    // cycle.
    expect(that % 1 == hal::cycles_per(100'000 * si::hertz, 1us));
    expect(that % 1 == hal::cycles_per(100 * si::hertz, 1ms));
  };

  "cycles_per rounds up, so the cycles are never shorter"_test = []() {
    // Exercise
    // 1.5 s at 3 Hz is 4.5 cycles, which counts the whole seconds and the
    // rest of a second separately.
    auto const split = hal::cycles_per(3 * si::hertz, 1'500ms);
    // 1,001 ns at 8 MHz is 8.008 cycles.
    auto const odd = hal::cycles_per(8'000'000 * si::hertz, 1'001ns);
    // 1 ns at 8 MHz is 0.008 of a cycle.
    auto const tiny = hal::cycles_per(8'000'000 * si::hertz, 1ns);

    // Verify
    expect(that % 5 == split);
    expect(that % 9 == odd);
    expect(that % 1 == tiny);
  };

  "cycles_per doesn't overflow for long durations"_test = []() {
    // Setup
    constexpr auto max_u32 = std::numeric_limits<hal::u32>::max();
    constexpr auto max_seconds = std::chrono::seconds(max_u32);

    // Exercise
    // 10 minutes at 72 MHz, past what the whole duration times the frequency
    // fits in 64 bits
    auto const minutes = hal::cycles_per(72'000'000 * si::hertz, 10min);
    // The largest clock for the most whole seconds that are counted, with the
    // rest of a second on top, is the largest count that doesn't saturate.
    auto const largest_count =
      hal::cycles_per(max_u32 * si::hertz, max_seconds + 999'999'999ns);

    // Verify
    expect(that % 43'200'000'000ULL == minutes);
    // (2^32 - 1)^2 cycles for the seconds, 4,294,967,291 for the rest
    expect(that % 18'446'744'069'414'584'316ULL == largest_count);
  };

  "cycles_per is 0 for a duration that isn't positive"_test = []() {
    // Exercise
    auto const zero = hal::cycles_per(1'000'000 * si::hertz, 0ms);
    auto const negative = hal::cycles_per(1'000'000 * si::hertz, -1ms);

    // Verify
    expect(that % 0 == zero);
    expect(that % 0 == negative);
  };

  "cycles_per saturates at 2^32 seconds"_test = []() {
    // Setup
    constexpr auto max_u32 = std::numeric_limits<hal::u32>::max();
    constexpr auto max_u64 = std::numeric_limits<hal::u64>::max();

    // Exercise
    auto const below_limit =
      hal::cycles_per(1 * si::hertz, std::chrono::seconds(max_u32));
    auto const at_limit = hal::cycles_per(
      1 * si::hertz, std::chrono::seconds(hal::u64{ max_u32 } + 1));
    auto const largest =
      hal::cycles_per(max_u32 * si::hertz, hal::time_duration::max());

    // Verify
    expect(that % max_u32 == below_limit);
    expect(that % max_u64 == at_limit);
    expect(that % max_u64 == largest);
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
           hal::duration_from_cycles(1'000'000 * si::hertz, 1400));
    expect(that % 10'250ms ==
           hal::duration_from_cycles(1'000 * si::hertz, 10'250));
    expect(that % 12'000us ==
           hal::duration_from_cycles(1'000'000'000 * si::hertz, 12'000'000));
    expect(that % 1ns ==
           hal::duration_from_cycles(1'000'000'000 * si::hertz, 1));
    expect(that % 1us == hal::duration_from_cycles(1'000'000 * si::hertz, 1));
    expect(that % 1ms == hal::duration_from_cycles(1'000 * si::hertz, 1));
    expect(that % 1s == hal::duration_from_cycles(1 * si::hertz, 1));
    expect(that % 2'000'000us ==
           hal::duration_from_cycles(500'000 * si::hertz, 1'000'000));
    expect(that % 25ms ==
           hal::duration_from_cycles(2'000'000'000 * si::hertz, 50'000'000));
    expect(that % 10min ==
           hal::duration_from_cycles(72'000'000 * si::hertz, 43'200'000'000));
  };

  "duration_from_cycles rounds up, so the duration is never shorter"_test =
    []() {
      // 33,333 cycles at 14 MHz take 2,380,928.57 ns
      expect(that % 2'380'929ns ==
             hal::duration_from_cycles(14'000'000 * si::hertz, 33'333));
      // One cycle at 3 Hz takes 333,333,333.3 ns
      expect(that % 333'333'334ns ==
             hal::duration_from_cycles(3 * si::hertz, 1));
      // 4 cycles at 3 Hz: a whole second, plus the rest rounded up
      expect(that % 1'333'333'334ns ==
             hal::duration_from_cycles(3 * si::hertz, 4));
    };

  "duration_from_cycles and cycles_per are the reverse of each other"_test =
    []() {
      for (auto const frequency : { 3U, 1'000U, 14'000'000U, 72'000'000U }) {
        for (hal::u64 const cycles : { 1ULL, 7ULL, 33'333ULL, 1'000'003ULL }) {
          auto const duration =
            hal::duration_from_cycles(frequency * si::hertz, cycles);
          // Rounding up both ways can add at most one cycle.
          auto const back = hal::cycles_per(frequency * si::hertz, duration);
          expect(that % cycles <= back);
          expect(that % back <= (cycles + 1));
        }
      }
    };

  "duration_from_cycles is 0 for no cycles"_test = []() {
    expect(that % 0ns == hal::duration_from_cycles(1'000'000 * si::hertz, 0));
    expect(that % 0ns == hal::duration_from_cycles(0 * si::hertz, 0));
  };

  "duration_from_cycles never ends at 0 Hz"_test = []() {
    expect(hal::time_duration::max() ==
           hal::duration_from_cycles(0 * si::hertz, 1));
  };

  "duration_from_cycles saturates without overflowing"_test = []() {
    constexpr auto max_u64 = std::numeric_limits<hal::u64>::max();
    constexpr auto max_u32 = std::numeric_limits<hal::u32>::max();
    constexpr auto max_counts =
      static_cast<hal::u64>(hal::time_duration::max().count());
    constexpr hal::u64 max_seconds = max_counts / 1'000'000'000;

    expect(hal::time_duration::max() ==
           hal::duration_from_cycles(1 * si::hertz, max_u64));
    expect(hal::time_duration::max() ==
           hal::duration_from_cycles(1 * si::hertz, max_seconds + 1));
    // From the last whole second that fits on, it saturates
    expect(hal::time_duration::max() ==
           hal::duration_from_cycles(1 * si::hertz, max_seconds));
    expect(that % std::chrono::seconds(max_seconds - 1) ==
           hal::duration_from_cycles(1 * si::hertz, max_seconds - 1));
    // The second before it, plus most of a second, rounded up
    expect(
      that % (std::chrono::seconds(max_seconds - 1) + 900ms) ==
      hal::duration_from_cycles(10 * si::hertz, ((max_seconds - 1) * 10) + 9));
    // The largest frequency with the most cycles left over: 999,999,999.77 ns
    // rounds up to a whole second.
    expect(that % 1'001s == hal::duration_from_cycles(
                              max_u32 * si::hertz,
                              (hal::u64{ max_u32 } * 1000) + (max_u32 - 1)));
  };
}

}  // namespace

int main()
{
  units_test();
}
