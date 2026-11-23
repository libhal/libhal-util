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

module;

#include <concepts>
#include <cstddef>
#include <cstdint>

export module hal.util:overflow_counter;

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
export template<size_t count_bit_width = 32>
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
  constexpr uint64_t update(uint32_t p_new_count)
  {
    // Sanitize the new count value to make sure it does not exceed the
    // designated bit width. Without this check when the count is combined
    // with the overflow value the upper bits may effect the bits of the
    // overflow count, getting an incorrect count value.
    constexpr auto mask = generate_field_of_ones<count_bit_width, uint32_t>();
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
    uint64_t combined_count = m_overflow_count;
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
  uint32_t m_previous_count = 0;
  uint32_t m_overflow_count = 0;
};

/**
 * @ingroup OverflowCounter
 * @brief Count laps of a hardware counter that wraps, and read the total count
 * across laps
 *
 * For hardware that counts through a fixed length and starts over on its own,
 * with an interrupt for each finished lap. Examples are a circular DMA receive,
 * whose lap is the buffer, and a timer, whose lap is its reload value plus one.
 * The interrupt calls `count_lap()`, and `total()` combines the laps with the
 * position in the current lap into a count that keeps growing.
 *
 * Unlike `hal::overflow_counter`, the hardware doesn't have to be read at
 * least once per lap. `total()` is also right while the interrupt can't run,
 * such as with interrupts disabled: a lap the hardware has finished, but the
 * interrupt hasn't counted yet, is added to the result.
 *
 * The lap length isn't stored: `total()` takes it, because the driver keeps
 * it anyway or can read it back from the hardware (a timer's reload value).
 *
 * Requirements on the hardware:
 * - The hardware sets its lap flag no later than its position starts over.
 *   Otherwise a read in between sees a new lap's position with the flag still
 *   clear, and the total comes out one lap short.
 * - A lap takes much longer than one `total()` read of the flag, the position
 *   and the flag again. Otherwise every read sees a lap end, and `total()`
 *   never returns.
 *
 * @pre Everything runs on one core. An interrupt on another core can change
 * the hardware between reads without `total()` noticing.
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
 */
export class lap_counter
{
public:
  constexpr lap_counter() = default;
  lap_counter(lap_counter const&) = delete;
  lap_counter& operator=(lap_counter const&) = delete;
  lap_counter(lap_counter&&) = delete;
  lap_counter& operator=(lap_counter&&) = delete;
  ~lap_counter() = default;

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
   * Reads the lap count, the lap flag and the position again until no lap
   * ended during the read. A position that moves between reads is fine: the
   * result is the total at the moment of the last position read.
   *
   * @pre Not called from an interrupt that can preempt the lap interrupt
   * between `count_lap()` and its flag clear. It would count that lap twice.
   * The lap interrupt can rule this out by calling `count_lap()` and clearing
   * the flag with interrupts masked.
   *
   * @tparam Count - the type of the result, `hal::u32` unless given. The
   * result wraps at its maximum, so differences between two results stay
   * right with unsigned arithmetic.
   * @param p_lap_length - how many counts the hardware goes through before it
   * starts over. For a circular DMA receive, the buffer length. For a timer,
   * its reload value plus one.
   * @param p_lap_finished - returns true if the hardware's lap flag is set: a
   * lap has finished, but the interrupt hasn't counted it yet
   * @param p_position_in_lap - returns, as a `hal::u32`, how far the hardware
   * is into the current lap, counting up from 0 to the lap length minus one
   * @return Count - the total count since `reset()`
   */
  template<std::unsigned_integral Count = hal::u32>
  [[nodiscard]] Count total(
    hal::u32 p_lap_length,
    hal::callable<bool()> auto&& p_lap_finished,
    hal::callable<hal::u32()> auto&& p_position_in_lap) const
  {
    hal::u32 laps = 0;
    Count position = 0;
    bool finished = false;
    bool finished_during_read = false;
    do {
      laps = m_laps;
      bool const finished_before = p_lap_finished();
      position = static_cast<Count>(p_position_in_lap());
      finished = p_lap_finished();
      finished_during_read = (finished_before != finished);
    } while ((laps != m_laps) or finished_during_read);

    // The hardware has started the next lap, so the position belongs to the
    // lap after the finished one. Counting it is left to the interrupt.
    auto const completed_laps =
      finished ? (static_cast<Count>(laps) + 1U) : static_cast<Count>(laps);
    return static_cast<Count>((completed_laps * p_lap_length) + position);
  }

private:
  hal::u32 volatile m_laps = 0;
};
}  // namespace hal::inline v6
