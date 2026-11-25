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

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <type_traits>

#include <boost/ut.hpp>

import hal.util.bit_ranges;

namespace {
void set_bits_view_test()
{
  using namespace boost::ut;
  using u32_view = hal::set_bits_view<std::uint32_t>;

  static_assert(std::forward_iterator<u32_view::iterator>);
  static_assert(std::ranges::view<u32_view>);
  static_assert(std::ranges::forward_range<u32_view>);
  static_assert(std::ranges::common_range<u32_view>);
  static_assert(std::ranges::sized_range<u32_view>);
  static_assert(std::ranges::borrowed_range<u32_view>);

  "hal::set_bits_view visits each 1, lowest first"_test = []() {
    // Setup
    std::array<std::uint32_t, 4> const expected = { 2U, 4U, 5U, 7U };
    std::array<std::uint32_t, 4> visited{};
    std::size_t count = 0;

    // Exercise
    for (auto const position : hal::set_bits_view(0b1011'0100U)) {
      if (count < visited.size()) {
        visited[count] = position;
      }
      count++;
    }

    // Verify
    expect(that % expected.size() == count);
    expect(expected == visited);
  };

  "hal::set_bits_view is empty for 0"_test = []() {
    // Setup
    hal::set_bits_view const positions(std::uint32_t{ 0 });
    std::size_t count = 0;

    // Exercise
    for ([[maybe_unused]] auto const position : positions) {
      count++;
    }

    // Verify
    expect(positions.begin() == positions.end());
    expect(that % 0U == count);
  };

  "hal::set_bits_view reaches the top bit of every width"_test = []() {
    // Setup
    constexpr auto last_position = [](auto p_value) {
      std::uint32_t last = 0;
      for (auto const position : hal::set_bits_view(p_value)) {
        last = position;
      }
      return last;
    };

    // Exercise
    constexpr auto u8_last = last_position(std::uint8_t{ 0x80 });
    constexpr auto u16_last = last_position(std::uint16_t{ 0x8001 });
    constexpr auto u64_last = last_position(std::uint64_t{ 1 } << 63U);
    constexpr auto u32_count =
      std::ranges::distance(hal::set_bits_view(0xFFFF'FFFFU));

    // Verify
    expect(that % 7U == u8_last);
    expect(that % 15U == u16_last);
    expect(that % 63U == u64_last);
    expect(that % 32 == u32_count);
  };

  "hal::set_bits_view keeps its own copy of the value"_test = []() {
    // Setup
    std::uint32_t volatile pending = 0b0101U;
    hal::set_bits_view const positions(pending);
    std::uint32_t rebuilt = 0;

    // Exercise
    pending = 0U;
    for (auto const position : positions) {
      rebuilt = rebuilt | (1U << position);
    }

    // Verify
    expect(that % 0b0101U == rebuilt);
  };

  "hal::set_bits_view works with standard views and algorithms"_test = []() {
    // Setup
    constexpr std::uint32_t pending = 0b1011'0100U;

    // Exercise
    auto const first_two = hal::set_bits_view(pending) | std::views::take(2);
    auto const found = std::ranges::find(hal::set_bits_view(pending), 5U);
    auto const size = hal::set_bits_view(pending).size();
    auto const front = hal::set_bits_view(pending).front();
    bool const empty = hal::set_bits_view(std::uint32_t{ 0 }).empty();
    std::array<std::uint32_t, 2> taken{};
    std::ranges::copy(first_two, taken.begin());

    // Verify
    expect((std::array<std::uint32_t, 2>{ 2U, 4U }) == taken);
    // A borrowed range: the iterator outlives the view it came from
    expect(that % 5U == *found);
    expect(that % 4U == size);
    expect(that % 2U == front);
    expect(empty);
  };

  "hal::views::set_bits makes the same view as hal::set_bits_view"_test = []() {
    // Setup
    constexpr std::uint16_t pending = 0b1000'0000'0000'0110U;
    static_assert(std::is_same_v<decltype(hal::views::set_bits(pending)),
                                 hal::set_bits_view<std::uint16_t>>);
    // Like the view's constructor, it only takes unsigned values
    static_assert(not std::invocable<decltype(hal::views::set_bits), int>);
    std::array<std::uint32_t, 3> visited{};
    std::size_t count = 0;

    // Exercise
    for (auto const position : hal::views::set_bits(pending)) {
      if (count < visited.size()) {
        visited[count] = position;
      }
      count++;
    }
    constexpr auto size = hal::views::set_bits(pending).size();

    // Verify
    expect(that % 3U == count);
    expect((std::array<std::uint32_t, 3>{ 1U, 2U, 15U }) == visited);
    expect(that % 3U == size);
  };

  "hal::set_bits_view rebuilds every u8 in order"_test = []() {
    for (unsigned raw = 0; raw <= 0xFFU; raw++) {
      // Setup
      auto const value = static_cast<std::uint8_t>(raw);
      unsigned rebuilt = 0;
      int count = 0;
      std::uint32_t lowest_allowed = 0;
      bool ascending = true;

      // Exercise
      for (auto const position : hal::set_bits_view(value)) {
        ascending = ascending and (position >= lowest_allowed);
        lowest_allowed = position + 1U;
        rebuilt = rebuilt | (1U << position);
        count++;
      }

      // Verify
      expect(that % raw == rebuilt) << "value: " << raw;
      expect(that % std::popcount(value) == count) << "value: " << raw;
      expect(ascending) << "value: " << raw;
    }
  };
}
}  // namespace

int main()
{
  set_bits_view_test();
}
