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

module;

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>

/**
 * @file
 * @brief Standard ranges over the bits of a value.
 *
 * A separate module from `hal.util`, because it includes `<ranges>`, a large
 * header that most users of `hal.util` don't need. Import it on its own:
 *
 *     import hal.util.bit_ranges;
 */
export module hal.util.bit_ranges;

namespace hal::inline v6 {
/**
 * @ingroup Bit
 * @brief A view of the positions of the bits that are 1 in a value, from the
 * lowest to the highest.
 *
 * Use it in a range-based for loop to handle each pending flag in a status
 * register, such as the interrupt lines that are waiting:
 *
 *     auto const pending = reg->pending;
 *     for (auto const line : hal::views::set_bits(pending)) {
 *       handle(line);
 *     }
 *
 * As with the standard views, `hal::views::set_bits(pending)` and
 * `hal::set_bits_view(pending)` make the same view, the way
 * `std::views::iota(0, 5)` and `std::ranges::iota_view(0, 5)` do.
 *
 * The loop runs once for each bit that is 1, not once for each bit position.
 * Each step is a count of trailing zeros and `bits & (bits - 1)`, which is
 * cheaper than clearing the bit at the position it found.
 *
 * It models `std::ranges::view`, `std::ranges::forward_range`,
 * `std::ranges::sized_range` and `std::ranges::borrowed_range`, so it works
 * with the standard range algorithms and views, like `std::ranges::iota_view`.
 * The positions are `std::uint32_t`, the type of `hal::bit_mask::position`.
 *
 * Import the `hal.util.bit_ranges` module to use it.
 *
 * The view holds a copy of the value, so later changes to the variable or
 * register it came from don't change it. Its iterators hold their own copy
 * of the bits too, so they stay valid after the view is destroyed.
 *
 * @tparam T - the unsigned integral type of the value
 */
export template<std::unsigned_integral T>
class set_bits_view : public std::ranges::view_interface<set_bits_view<T>>
{
public:
  /**
   * @brief Steps through the bits that are 1, lowest first.
   *
   * It holds the bits it hasn't reached yet. When none are left, it equals the
   * end of the view.
   */
  class iterator
  {
  public:
    /// The iterator's category: it can be copied and read more than once
    using iterator_concept = std::forward_iterator_tag;
    /// The iterator's category, for algorithms from before C++20
    using iterator_category = std::forward_iterator_tag;
    /// The type of a bit position, the same as `hal::bit_mask::position`
    using value_type = std::uint32_t;
    /// The type of the distance between two iterators
    using difference_type = std::ptrdiff_t;

    /**
     * @brief An iterator with no bits left, which equals the end of any view.
     */
    constexpr iterator() = default;

    /**
     * @brief An iterator at the lowest bit of the value that is 1.
     *
     * @param p_bits - the bits to step through
     */
    constexpr explicit iterator(T p_bits)
      : m_bits(p_bits)
    {
    }

    /**
     * @brief The position of the lowest bit that is 1.
     *
     * @return the bit position, from 0 for the least significant bit
     */
    [[nodiscard]] constexpr value_type operator*() const
    {
      return static_cast<value_type>(std::countr_zero(m_bits));
    }

    /**
     * @brief Move to the next bit that is 1.
     *
     * @return this iterator
     */
    constexpr iterator& operator++()
    {
      m_bits = static_cast<T>(m_bits & (m_bits - 1U));
      return *this;
    }

    /**
     * @brief Move to the next bit that is 1.
     *
     * @return a copy of this iterator from before it moved
     */
    constexpr iterator operator++(int)
    {
      auto const before = *this;
      ++*this;
      return before;
    }

    /**
     * @brief Whether two iterators have the same bits left.
     *
     * @param p_other - the iterator to compare with
     * @return true if both have the same bits left
     */
    constexpr bool operator==(iterator const& p_other) const = default;

  private:
    T m_bits = 0;
  };

  /**
   * @brief A view with no bits that are 1.
   */
  constexpr set_bits_view() = default;

  /**
   * @brief A view of the bits of a value that are 1.
   *
   * @param p_value - the value whose bits to visit
   */
  constexpr explicit set_bits_view(T p_value)
    : m_value(p_value)
  {
  }

  /**
   * @brief An iterator at the lowest bit that is 1.
   *
   * @return the first iterator, equal to `end()` if no bit is 1
   */
  [[nodiscard]] constexpr iterator begin() const
  {
    return iterator{ m_value };
  }

  /**
   * @brief The end of the view, reached after the highest bit that is 1.
   *
   * @return an iterator with no bits left
   */
  [[nodiscard]] constexpr iterator end() const
  {
    return iterator{};
  }

  /**
   * @brief The number of bits that are 1.
   *
   * @return the number of positions the view visits
   */
  [[nodiscard]] constexpr std::size_t size() const
  {
    return static_cast<std::size_t>(std::popcount(m_value));
  }

private:
  T m_value = 0;
};

namespace views {
/**
 * @ingroup Bit
 * @brief The type of `hal::views::set_bits`
 */
struct set_bits_fn
{
  /**
   * @brief Make a view of the positions of a value's bits that are 1
   *
   * @tparam T - the unsigned integral type of the value
   * @param p_value - the value whose bits to visit
   * @return `hal::set_bits_view<T>(p_value)`
   */
  template<std::unsigned_integral T>
  [[nodiscard]] constexpr set_bits_view<T> operator()(T p_value) const
  {
    return set_bits_view<T>(p_value);
  }
};

/**
 * @ingroup Bit
 * @brief Make a `hal::set_bits_view`, as `std::views::iota` makes a
 * `std::ranges::iota_view`
 *
 *     for (auto const line : hal::views::set_bits(pending)) {
 *       handle(line);
 *     }
 */
export inline constexpr set_bits_fn set_bits{};
}  // namespace views
}  // namespace hal::inline v6

/// set_bits_view's iterators hold their own copy of the bits, so they stay
/// valid after the view is destroyed, as std::ranges::iota_view's do.
template<std::unsigned_integral T>
inline constexpr bool
  std::ranges::enable_borrowed_range<hal::set_bits_view<T>> = true;
