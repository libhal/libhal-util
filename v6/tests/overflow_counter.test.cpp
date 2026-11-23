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

#include <type_traits>

#include <boost/ut.hpp>

import hal;
import hal.util;

namespace {

void update_test()
{
  using namespace boost::ut;

  "overflow_counter::update() increment by 1"_test = []() {
    hal::overflow_counter counter;

    expect(that % 0 == counter.update(0));
    expect(that % 1 == counter.update(1));
    expect(that % 2 == counter.update(2));
    expect(that % 3 == counter.update(3));
    expect(that % 4 == counter.update(4));
    expect(that % 5 == counter.update(5));
    expect(that % 6 == counter.update(6));
  };

  // This is the case when a counter has been stopped. It shouldn't detect an
  // overflow as the value hasn't overflowed it just has not moved.
  "overflow_counter::update() increment by 0 expect no change"_test = []() {
    hal::overflow_counter counter;

    expect(that % 10 == counter.update(10));
    expect(that % 10 == counter.update(10));
    expect(that % 10 == counter.update(10));
    expect(that % 10 == counter.update(10));
    expect(that % 10 == counter.update(10));
    expect(that % 10 == counter.update(10));
    expect(that % 10 == counter.update(10));
  };

  // This is the case when a counter has been stopped. It shouldn't detect an
  // overflow as the value hasn't overflowed it just has not moved.
  "overflow_counter::update() overflow w/ 32 bits"_test = []() {
    hal::overflow_counter counter;

    expect(that % 0x0'0000'000A == counter.update(0xA));
    expect(that % 0x1'0000'0000 == counter.update(0x0));
    expect(that % 0x1'0000'000A == counter.update(0xA));

    expect(that % 0x2'0000'0000 == counter.update(0x0));
    expect(that % 0x2'0000'000A == counter.update(0xA));

    expect(that % 0x3'0000'0000 == counter.update(0x0));
    expect(that % 0x3'0000'000A == counter.update(0xA));

    expect(that % 0x4'0000'0000 == counter.update(0x0));
    expect(that % 0x4'0000'000A == counter.update(0xA));

    expect(that % 0x5'0000'0000 == counter.update(0x0));
    expect(that % 0x5'0000'000A == counter.update(0xA));
    expect(that % 0x5'0000'000B == counter.update(0xB));
    expect(that % 0x5'0000'000C == counter.update(0xC));
    expect(that % 0x5'0000'000D == counter.update(0xD));
    expect(that % 0x5'0000'000E == counter.update(0xE));
    expect(that % 0x5'0000'000F == counter.update(0xF));
    expect(that % 0x6'0000'0000 == counter.update(0x0));
  };

  "overflow_counter::update() overflow w/ 8 bits"_test = []() {
    hal::overflow_counter<8> counter;

    expect(that % 0x00A == counter.update(0xA));
    expect(that % 0x100 == counter.update(0x0));
    expect(that % 0x10A == counter.update(0xA));

    expect(that % 0x200 == counter.update(0x0));
    expect(that % 0x20A == counter.update(0xA));

    expect(that % 0x300 == counter.update(0x0));
    expect(that % 0x30A == counter.update(0xA));

    expect(that % 0x400 == counter.update(0x0));
    expect(that % 0x40A == counter.update(0xA));

    expect(that % 0x500 == counter.update(0x0));
    expect(that % 0x50A == counter.update(0xA));
    expect(that % 0x50B == counter.update(0xB));
    expect(that % 0x50C == counter.update(0xC));
    expect(that % 0x50D == counter.update(0xD));
    expect(that % 0x50E == counter.update(0xE));
    expect(that % 0x50F == counter.update(0xF));
    expect(that % 0x600 == counter.update(0x0));
  };

  "overflow_counter::update() reset"_test = []() {
    hal::overflow_counter<8> counter;

    expect(that % 0x00A == counter.update(0xA));
    expect(that % 0x100 == counter.update(0x0));
    expect(that % 0x10A == counter.update(0xA));

    expect(that % 0x200 == counter.update(0x0));
    expect(that % 0x20A == counter.update(0xA));

    expect(that % 0x300 == counter.update(0x0));
    expect(that % 0x30A == counter.update(0xA));

    expect(that % 0x400 == counter.update(0x0));
    expect(that % 0x40A == counter.update(0xA));

    counter.reset();

    expect(that % 0x00A == counter.update(0xA));
  };
}

void lap_counter_test()
{
  using namespace boost::ut;

  "lap_counter::total() starts at the position after reset()"_test = []() {
    // Setup
    hal::lap_counter laps;
    laps.reset();

    // Exercise
    auto const total =
      laps.total(10, [] { return false; }, [] { return hal::u32{ 5 }; });

    // Verify
    expect(that % 5U == total);
  };

  "lap_counter::total() counts laps from count_lap()"_test = []() {
    // Setup
    hal::lap_counter laps;
    laps.reset();
    laps.count_lap();
    laps.count_lap();
    laps.count_lap();

    // Exercise
    auto const total =
      laps.total(10, [] { return false; }, [] { return hal::u32{ 5 }; });

    // Verify
    expect(that % 35U == total);
  };

  "lap_counter::reset() goes back to 0"_test = []() {
    // Setup
    hal::lap_counter laps;
    laps.reset();
    laps.count_lap();
    laps.reset();
    laps.count_lap();

    // Exercise
    auto const total =
      laps.total(4, [] { return false; }, [] { return hal::u32{ 1 }; });

    // Verify
    expect(that % 5U == total);
  };

  "lap_counter::total() adds a lap the interrupt hasn't counted"_test = []() {
    // Setup
    hal::lap_counter laps;
    laps.reset();
    laps.count_lap();
    laps.count_lap();
    laps.count_lap();

    // Exercise
    auto const total =
      laps.total(10, [] { return true; }, [] { return hal::u32{ 1 }; });
    auto const after_interrupt =
      laps.total(10, [] { return false; }, [] { return hal::u32{ 1 }; });

    // Verify
    expect(that % 41U == total);
    // total() left the lap to the interrupt
    expect(that % 31U == after_interrupt);
  };

  "lap_counter::total() reads again if the interrupt ran during the read"_test =
    []() {
      // Setup
      hal::lap_counter laps;
      laps.reset();
      laps.count_lap();
      laps.count_lap();
      laps.count_lap();
      unsigned position_reads = 0;
      auto const position = [&] {
        position_reads++;
        if (position_reads == 1) {
          // The interrupt counts a lap and clears the flag mid-read
          laps.count_lap();
          return hal::u32{ 9 };
        }
        return hal::u32{ 2 };
      };

      // Exercise
      auto const total = laps.total(10, [] { return false; }, position);

      // Verify
      expect(that % 42U == total);
      expect(that % 2U == position_reads);
    };

  "lap_counter::total() reads again if a lap finished during the read"_test =
    []() {
      // Setup
      hal::lap_counter laps;
      laps.reset();
      laps.count_lap();
      laps.count_lap();
      laps.count_lap();
      unsigned flag_reads = 0;
      // Clear before the first position read, set after it, then stays set
      auto const finished = [&] {
        flag_reads++;
        return flag_reads >= 2;
      };
      unsigned position_reads = 0;
      auto const position = [&] {
        position_reads++;
        return (position_reads == 1) ? hal::u32{ 9 } : hal::u32{ 0 };
      };

      // Exercise
      auto const total = laps.total(10, finished, position);

      // Verify
      expect(that % 40U == total);
      expect(that % 2U == position_reads);
    };

  "lap_counter::total() takes a position that moves on every read"_test = []() {
    // Setup
    hal::lap_counter laps;
    laps.reset();
    laps.count_lap();
    laps.count_lap();
    laps.count_lap();
    // A fast timer: each read is one count further
    hal::u32 position_reads = 0;
    auto const position = [&] {
      position_reads++;
      return hal::u32{ 4 } + position_reads;
    };

    // Exercise
    auto const total = laps.total(10, [] { return false; }, position);

    // Verify
    // No lap ended during the read, so the first position read is used
    expect(that % 35U == total);
    expect(that % 1U == position_reads);
  };

  "lap_counter::total() wider result type"_test = []() {
    // Setup
    hal::lap_counter laps;
    laps.reset();
    for (int i = 0; i < 0x1'0001; i++) {
      laps.count_lap();
    }

    // Exercise
    auto const total = laps.total<hal::u64>(
      0x1'0000, [] { return false; }, [] { return hal::u32{ 0x10 }; });

    // Verify
    static_assert(std::is_same_v<decltype(total), hal::u64 const>);
    expect(that % 0x1'0001'0010ULL == total);
  };

  "lap_counter::total() differences stay right when the result wraps"_test =
    []() {
      // Setup
      hal::lap_counter laps;
      // Three laps of 0x5555'5555 counts reach the u32 maximum
      laps.reset();
      laps.count_lap();
      laps.count_lap();
      auto const before = laps.total(
        0x5555'5555,
        [] { return false; },
        [] { return hal::u32{ 0x5555'5554 }; });
      laps.count_lap();

      // Exercise
      auto const after = laps.total(
        0x5555'5555, [] { return false; }, [] { return hal::u32{ 3 }; });

      // Verify
      expect(that % 4U == static_cast<hal::u32>(after - before));
    };

  "lap_counter can be constant initialized"_test = []() {
    // Setup
    static constinit hal::lap_counter laps{};
    laps.reset();

    // Exercise
    auto const total =
      laps.total(8, [] { return false; }, [] { return hal::u32{ 7 }; });

    // Verify
    expect(that % 7U == total);
  };
}
}  // namespace

int main()
{
  update_test();
  lap_counter_test();
}
