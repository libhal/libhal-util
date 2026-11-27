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

export module hal.util:overflow_counter;

import std;

import hal;
import :bit;

/**
 * @defgroup OverflowCounter Overflow Counter
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup OverflowCounter
 * @brief Extend a counter's count from an arbitrary bit width to 64-bits by
 * detecting overflows in the count. Each detected overflow is added to an
 * overflow counter which is combined with the current count in order create a
 * count up to 64-bits in length.
 *
 * @tparam count_bit_width - the bit width of the counter before the count
 * value overflows.
 */
export template<std::size_t count_bit_width = 32>
class overflow_counter
{
public:
  static_assert(count_bit_width <= 32, "Bit width cannot exceed 32-bits");
  static_assert(count_bit_width > 1, "Bit width must be greater than 1");

  /**
   * @ingroup OverflowCounter
   * @brief update the overflow counter, detect if an overflow has occurred,
   * and return the combined
   *
   * @param p_new_count - must be an increasing value and should only decrease
   * when an overflow event occurs.
   * @return constexpr uint64_t - 64-bit count combining the new count value
   * and the overflow count value.
   */
  constexpr std::uint64_t update(std::uint32_t p_new_count)
  {
    // Sanitize the new count value to make sure it does not exceed the
    // designated bit width. Without this check when the count is combined
    // with the overflow value the upper bits may effect the bits of the
    // overflow count, getting an incorrect count value.
    constexpr auto mask =
      generate_field_of_ones<count_bit_width, std::uint32_t>();
    p_new_count = p_new_count & mask;

    // Detect the overflow by checking if the new count is smaller than the
    // previous count. If the count is always increasing, the only way for
    // the new count to be smaller is if the count reached the end of its
    // bit width and overflowed.
    if (m_previous_count > p_new_count) {
      m_overflow_count++;
    }

    // Set previous count to the new count
    m_previous_count = p_new_count;

    // Move overflow count up to the upper bits of the 64-bit number based on
    // count_bit_width
    std::uint64_t combined_count = m_overflow_count;
    combined_count <<= count_bit_width;
    // Add the p_new_count into the combined count to complete it.
    combined_count |= p_new_count;

    return combined_count;
  }

  /**
   * @ingroup OverflowCounter
   * @brief Reset the overflow count back to zero.
   *
   */
  constexpr void reset()
  {
    m_previous_count = 0;
    m_overflow_count = 0;
  }

private:
  std::uint32_t m_previous_count = 0;
  std::uint32_t m_overflow_count = 0;
};

/**
 * @ingroup OverflowCounter
 * @brief Count a hardware counter's laps from the interrupt that fires at the
 * end of each lap, and read the total count across all laps
 *
 * Some hardware counts from 0 up to a fixed length, then starts over at 0 and
 * sets a flag that raises an interrupt. That's one lap. Examples of this are:
 *
 * - A circular DMA receive. The DMA writes bytes into a buffer and starts over
 *   at the beginning when it reaches the end. A lap is the buffer length, the
 *   position is how many bytes it has written in the current lap, and the flag
 *   is its transfer complete flag.
 * - A steady clock using a timer that generates an interrupt when it reaches
 *   the end of its count. A lap is the timer's reload value plus one, the
 *   position is its counter, and the flag is its update (overflow) flag.
 *
 * The lap interrupt calls `count_lap()`, then clears the flag. `total()`
 * returns the laps times the lap length, plus the position in the current lap.
 *
 * `total()` also counts a lap whose flag is set but whose interrupt hasn't run
 * yet. So it stays right with interrupts disabled, and unlike
 * `hal::overflow_counter`, the hardware doesn't have to be read once per lap.
 *
 * The hardware must:
 *
 * - Set the flag no later than it starts the next lap.
 *   Explanation: `total()` adds a lap when it sees the flag set. If the
 *   position starts over before the flag is set, a read in between sees the
 *   new lap's position without the flag, and the total comes out one lap short.
 *
 * `total()` returns after at most two reads, however short the laps are.
 *
 * `Laps` is the type of the lap count and the lap length. `hal::lap_counter`
 * counts laps in a `hal::u32`, which suits a `hal::u32` total, such as bytes
 * received. A 64-bit total needs `basic_lap_counter<hal::u64>`: with 32-bit
 * laps, a `hal::u64` total wraps back to 0 after 2^32 laps. A 16-bit timer
 * steady clock with no prescaler gets there in 45 days at 72 MHz. The 64-bit
 * lap length also allows a 2^32 count lap, such as two chained 16-bit timers.
 *
 * On a 32-bit core, reading a 64-bit lap count can tear when `count_lap()`
 * carries into its upper half. `total()` then sees two different lap counts
 * and reads again. A tear in that second read goes unnoticed, but it needs
 * the interrupt to count a lap during both reads, with laps about as short as
 * a read, and the carry to land on the second of them: once every 2^32 laps.
 *
 * @pre `count_lap()` and every `total()` call run on the same core.
 * Explanation: on another core, `total()` can read between `count_lap()` and
 * the flag clear. It sees the new lap count with the flag still set, and counts
 * that lap twice. Masking interrupts can't prevent this, because it only stops
 * interrupts on its own core. Without a memory barrier, another core can also
 * see the flag cleared before the new lap count, and come out one lap short.
 *
 * ```cpp
 * // Start of a circular DMA receive, before enabling its interrupt
 * laps.reset();
 *
 * // Transfer complete interrupt
 * laps.count_lap();
 * clear_transfer_complete_flag();
 *
 * // Bytes received since the start
 * auto const received = laps.total(
 *   buffer_length,
 *   [&] { return transfer_complete_flag_is_set(); },
 *   [&] { return buffer_length - remaining_transfers(); });
 * ```
 *
 * @tparam Laps - the lap count and lap length type: `hal::u32` or `hal::u64`
 */
export template<std::unsigned_integral Laps>
class basic_lap_counter
{
public:
  static_assert(std::is_same_v<Laps, hal::u32> or
                  std::is_same_v<Laps, hal::u64>,
                "Laps must be hal::u32 or hal::u64");

  constexpr basic_lap_counter() = default;
  basic_lap_counter(basic_lap_counter const&) = delete;
  basic_lap_counter& operator=(basic_lap_counter const&) = delete;
  basic_lap_counter(basic_lap_counter&&) = delete;
  basic_lap_counter& operator=(basic_lap_counter&&) = delete;
  ~basic_lap_counter() = default;

  /**
   * @brief Start over at a total of 0
   *
   * @pre The lap interrupt can't run, for example because it isn't enabled
   * yet.
   */
  void reset() noexcept
  {
    m_laps = 0;
  }

  /**
   * @brief Count one finished lap, from the lap interrupt
   *
   * @pre Called from the lap interrupt, before it clears the hardware flag
   * that says the lap finished. `total()` relies on that order to never miss
   * or double count a lap.
   *
   * The interrupt must run at least once per lap: a lap that finishes while the
   * flag is still set is lost, and the total comes out one lap short.
   */
  void count_lap() noexcept
  {
    m_laps = m_laps + 1U;
  }

  /**
   * @brief Read the total count since `reset()`
   *
   * The result is a total the count actually had at some moment during the
   * call, so results never go backwards. The position may move between reads.
   * One read is the lap count, the flag, the position, the flag again and the
   * lap count again:
   *
   * - Nothing changed during the read: the result is the laps, plus the lap
   *   the flag says has finished, plus the position.
   * - The flag got set during the read: a lap ended in between. The result is
   *   the total at that moment: the laps plus one, times the lap length.
   * - The interrupt counted a lap during the read: it reads once more. If the
   *   interrupt counts a lap during that read too, the result is the latest
   *   laps times the lap length. The count passed that value during the call,
   *   because two laps were counted after the call started.
   *
   * So with laps about as short as a read, the result can be a multiple of
   * the lap length without the position.
   *
   * @pre Not called from an interrupt that can preempt the lap interrupt
   * between `count_lap()` and its flag clear. It would count that lap twice.
   * The lap interrupt can rule this out by calling `count_lap()` and clearing
   * the flag with interrupts masked.
   *
   * @tparam Count - the type of the result, `Laps` unless given. The result
   * wraps at its maximum, so differences between two results stay right with
   * unsigned arithmetic, as long as `Count` is no wider than `Laps`.
   * @param p_lap_length - how many counts the hardware goes through before it
   * starts over. For a circular DMA receive, the buffer length. For a timer,
   * its reload value plus one. For two chained 16-bit timers, 2^32, which
   * needs `Laps = hal::u64`.
   * @param p_lap_finished - returns true if the hardware's lap flag is set: a
   * lap has finished, but the interrupt hasn't counted it yet
   * @param p_position_in_lap - returns, as a `hal::u32`, how far the hardware
   * is into the current lap, counting up from 0 to the lap length minus one
   * @return Count - the total count since `reset()`
   */
  template<std::unsigned_integral Count = Laps>
  [[nodiscard]] Count total(
    Laps p_lap_length,
    hal::callable<bool()> auto&& p_lap_finished,
    hal::callable<hal::u32()> auto&& p_position_in_lap) const
  {
    Laps laps = m_laps;
    Count completed_laps = 0;
    Count position = 0;
    for (int read = 0; read < 2; read++) {
      bool const finished_before = p_lap_finished();
      auto const position_read = static_cast<Count>(p_position_in_lap());
      bool const finished_after = p_lap_finished();
      Laps const laps_after = m_laps;

      // If the interrupt counts a lap during both reads, at least two laps
      // were counted after the call started. When the call started, the total
      // was below the first lap count plus two, times the lap length: at most
      // one lap was waiting for the interrupt, and the position is short of a
      // full lap. At the end it's at least the latest lap count times the lap
      // length, so the count passed that value during the call.
      completed_laps = static_cast<Count>(laps_after);
      position = 0;

      if (laps_after == laps) {
        // The hardware has started the next lap, so the position belongs to
        // the lap after the finished one. Counting it is left to the
        // interrupt.
        completed_laps = finished_after ? static_cast<Count>(laps + 1U)
                                        : static_cast<Count>(laps);
        // If a lap ended between the two flag reads, the position was 0 at
        // that moment
        position =
          (finished_before == finished_after) ? position_read : Count{ 0 };
        break;
      }
      laps = laps_after;
    }
    return static_cast<Count>(
      (completed_laps * static_cast<Count>(p_lap_length)) + position);
  }

private:
  Laps volatile m_laps = 0;
};

/**
 * @ingroup OverflowCounter
 * @brief A `hal::basic_lap_counter` with a 32-bit lap count, for 32-bit
 * totals
 */
export using lap_counter = basic_lap_counter<hal::u32>;
}  // namespace hal::inline v6
