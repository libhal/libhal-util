// Copyright 2024 Khalil Estell
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

#pragma once

#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>

/**
 * @defgroup Bit Bit Operations
 *
 */
namespace hal {
/**
 * @ingroup Bit
 * @brief Represents a bit mask of contiguous bits
 *
 */
struct bit_mask
{
  /// Where the bit mask starts
  std::uint32_t position;
  /// The number of bits after position contained in the mask
  std::uint32_t width;

  /**
   * @ingroup Bit
   * @brief Generate, at compile time, a bit_mask that spans the from position1
   * to position2.
   *
   * If position1 is the same position2 then the bit_mask will have length of 1
   * and the bit position will be the value of position1.
   *
   * position1 and position2 can be in any order so long as they span the
   * distance from the start and end of the bit_mask range.
   *
   * @tparam position1 - bit position 1
   * @tparam position2 - bit position 2
   * @return consteval bit_mask - bit bit_mask represented by the two bit
   * positions
   */
  template<std::uint32_t position1, std::uint32_t position2>
  static consteval bit_mask from()
  {
    constexpr std::uint32_t plus_one = 1;
    if constexpr (position1 < position2) {
      return bit_mask{ .position = position1,
                       .width = plus_one + (position2 - position1) };
    } else {
      return bit_mask{ .position = position2,
                       .width = plus_one + (position1 - position2) };
    }
  }

  /**
   * @ingroup Bit
   * @brief Generate, at compile time, a single bit width bit_mask at position
   *
   * @tparam position - the bit to make the bit_mask for
   * @return constexpr bit_mask - bit bit_mask with the position bit set to
   * position
   */
  template<std::uint32_t position>
  static constexpr bit_mask from()
  {
    return bit_mask{ .position = position, .width = 1U };
  }

  /**
   * @ingroup Bit
   * @brief Generate, at compile time, a bit_mask that spans the from position1
   * to position2.
   *
   * If position1 is the same position2 then the bit_mask will have length of 1
   * and the bit position will be the value of position1.
   *
   * position1 and position2 can be in any order so long as they span the
   * distance from the start and end of the bit_mask range.
   *
   * @param position1 - bit position 1
   * @param position2 - bit position 2
   * @return consteval bit_mask - bit bit_mask represented by the two bit
   * positions
   */
  static consteval bit_mask from(std::uint32_t position1,
                                 std::uint32_t position2)
  {
    constexpr std::uint32_t plus_one = 1;
    if (position1 < position2) {
      return bit_mask{ .position = position1,
                       .width = plus_one + (position2 - position1) };
    } else {
      return bit_mask{ .position = position2,
                       .width = plus_one + (position1 - position2) };
    }
  }

  /**
   * @ingroup Bit
   * @brief Generate, at runtime, a single bit width bit_mask at position
   *
   * @param position - the bit to make the bit_mask for
   * @return constexpr bit_mask - bit bit_mask with the position bit set to
   * position
   */
  static constexpr bit_mask from(std::uint32_t position)
  {
    return bit_mask{ .position = position, .width = 1U };
  }

  /**
   * @ingroup Bit
   * @brief Convert bit_mask to a integral representation but with bit position
   * at 0
   *
   * The integral presentation will have 1 bits starting from the position bit
   * up to bit position + width. All other bits will be 0s.
   *
   * For example:
   *
   *      value<std::uint16_t>(bit_mask{
   *          .position = 1,
   *          .width = 4,
   *      }); // returns = 0b0000'0000'0000'1111;
   *
   * @tparam T - unsigned integral type to hold the bit_mask
   * @return constexpr auto - bit_mask value as an unsigned integer
   */
  template<std::unsigned_integral T>
  constexpr auto origin() const
  {
    // At compile time, generate variable containing all 1s with the size of the
    // target parameter.
    constexpr T field_of_ones = std::numeric_limits<T>::max();

    // At compile time calculate the number of bits in the target parameter.
    constexpr size_t target_width = sizeof(T) * 8;

    // Create bit_mask by shifting the set of 1s down so that the number of 1s
    // from bit position 0 is equal to the width parameter.
    T mask_at_origin = static_cast<T>(field_of_ones >> (target_width - width));

    return mask_at_origin;
  }

  /**
   * @ingroup Bit
   * @brief Convert mask to a integral representation
   *
   * The integral presentation will have 1 bits starting from the position bit
   * up to bit position + width. All other bits will be 0s.
   *
   * For example:
   *
   *      value<std::uint16_t>(bit_mask{
   *          .position = 1,
   *          .width = 4,
   *      }); // returns = 0b0000'0000'0001'1110;
   *
   * @tparam T - unsigned integral type to hold the mask
   * @return constexpr auto - mask value as an unsigned integer
   */
  template<std::unsigned_integral T>
  constexpr auto value() const
  {
    return static_cast<T>(origin<T>() << position);
  }

  /**
   * @ingroup Bit
   * @brief Comparison operator between this mask and another
   *
   * @param other - the other mask to compare against
   * @return true - the masks are the same
   * @return false - the masks are not the same
   */
  constexpr bool operator==(bit_mask const& other)
  {
    return other.position == position && other.width == width;
  }
};

/**
 * @ingroup Bit
 * @brief Helper for generating byte position masks
 *
 * @tparam ByteIndex - the byte position to make a mask for
 */
template<size_t ByteIndex>
struct byte_mask
{
  /**
   * @ingroup Bit
   * @brief Mask value defined at compile time
   *
   */
  static constexpr hal::bit_mask value{ .position = ByteIndex, .width = 8 };
};

/**
 * @ingroup Bit
 * @brief Shorthand for using hal::byte_mask<N>::value
 *
 * @tparam ByteIndex - the byte position to make a mask for
 */
template<size_t ByteIndex>
constexpr hal::bit_mask byte_m = byte_mask<ByteIndex>::value;

/**
 * @ingroup Bit
 * @brief Helper for generating nibble position masks
 *
 * A nibble is considered 4 bits or half a byte's width in bits.
 *
 * @tparam NibbleIndex - the nibble position to make a mask for
 */
template<size_t NibbleIndex>
struct nibble_mask
{
  static constexpr hal::bit_mask value{ .position = NibbleIndex, .width = 4 };
};

/**
 * @ingroup Bit
 * @brief Shorthand for using hal::nibble_mask<N>::value
 *
 * @tparam NibbleIndex - the nibble position to make a mask for
 */
template<size_t NibbleIndex>
constexpr hal::bit_mask nibble_m = nibble_mask<NibbleIndex>::value;

/**
 * @ingroup Bit
 * @brief Extracts a specific field from an unsigned integral value using a
 * compile time (template) bit mask.
 *
 * Prefer to use this function over the non-template function if the bit mask
 * can be known at compile time. This will allow the compiler to optimize this
 * bit extract even further.
 *
 * USAGE:
 *
 *     static constexpr auto peripheral_state = hal::bit_mask::from<4, 9>();
 *     auto state = hal::bit_extract<peripheral_state>(reg->status);
 *     // Proceed to use `state` for something
 *
 * @tparam field - The bit mask defining the position and width of the field to
 * be extracted.
 * @param p_value The unsigned integral value from which the field will be
 * extracted.
 * @return A value representing only the specified field, with all other bits
 * set to zero.
 */
template<bit_mask field>
constexpr auto bit_extract(std::unsigned_integral auto p_value)
{
  using T = decltype(p_value);
  // Shift desired value to the right to position 0
  auto const shifted = p_value >> field.position;
  // Mask away any bits left of the value based on the field width
  auto const masked = shifted & field.origin<T>();
  // Leaving only the desired bits
  return static_cast<T>(masked);
}
/**
 * @ingroup Bit
 * @brief Extracts a specific field from an unsigned integral value using a bit
 * mask.
 *
 * If the bit mask is known and constant at compile time, use the
 * `hal::bit_extract<mask>` function as it provides more information to the
 * compiler that it can use to optimize the bit extraction operation.
 *
 * USAGE:
 *
 *     auto const peripheral_state = hal::bit_mask::from(4, 9);
 *     auto state = hal::bit_extract(peripheral_state, reg->status);
 *     // Proceed to use `state` for something
 *
 * @param p_field - The bit mask defining the position and width of the field to
 * be extracted.
 * @param p_value The unsigned integral value from which the field will be
 * extracted.
 * @return A value representing only the specified field, with all other bits
 * set to zero.
 */
constexpr auto bit_extract(bit_mask p_field,
                           std::unsigned_integral auto p_value)
{
  using T = decltype(p_value);
  // Shift desired value to the right to position 0
  auto const shifted = p_value >> p_field.position;
  // Mask away any bits left of the value based on the field width
  auto const masked = shifted & p_field.origin<T>();
  // Leaving only the desired bits
  return static_cast<T>(masked);
}

/**
 * @ingroup Bit
 * @brief A class for representing a value as a sequence of bits.
 *
 * This class provides a flexible way to manipulate and work with bit-level
 * values, allowing for efficient and expressive operations on large integers.
 *
 * This class provides template and non-template options for manipulating bits.
 * If the bitmask is KNOWN at COMPILE TIME, OPT to use the template versions of
 * the APIs as the compiler will have enough information to deduce the bit mask
 * and reduce the set of operations needed to perform the bit manipulation.
 *
 * USAGE:
 *
 *     static constexpr auto pre_scalar = hal::bit_mask::from<3, 18>();
 *     static constexpr auto enable = hal::bit_mask::from<19>();
 *     hal::bit_value my_value;
 *     my_value.insert<pre_scalar>(120UL)
 *        .set<enable>();
 *     reg->control = my_value.get();
 *
 * @tparam T - defaults to `std::uint32_t`, but can be any unsigned integral
 * type.
 */
template<std::unsigned_integral T = std::uint32_t>
class bit_value
{
public:
  /**
   * @brief The total number of bits in the represented value.
   *
   * This is calculated as the product of the size of `T` and the number of bits
   * per byte (`CHAR_BIT`).
   */
  static constexpr std::uint32_t width = sizeof(T) * CHAR_BIT;

  /**
   * @brief Constructs a new bit_value instance with an initial value.
   *
   * @param p_initial_value The initial value to use. Defaults to 0.
   */
  constexpr bit_value(T p_initial_value = 0)
    : m_value(p_initial_value)
  {
  }

  /**
   * @brief Sets (sets to a 1) multiple bits in the represented value.
   *
   * @tparam field A bit_mask type, which represents the position and size of
   * the bit to set.
   * @return A reference to this instance for method chaining.
   */
  template<bit_mask field>
  constexpr auto& set()
  {
    static_assert(field.position < width,
                  "Bit position exceeds register width");
    constexpr auto mask = field.value<T>();

    m_value = m_value | mask;

    return *this;
  }

  /**
   * @brief Sets (sets to a 1) multiple bits in the represented value.
   *
   * @param p_field A bit_mask instance, which represents the position and size
   * of the bit to set.
   * @return A reference to this instance for chaining.
   */
  constexpr auto& set(bit_mask p_field)
  {
    auto const mask = p_field.value<T>();

    m_value = m_value | mask;

    return *this;
  }

  /**
   * @brief Clears (sets to 0) multiple bits in the represented value.
   *
   * @tparam field A bit_mask type, which represents the position and size of
   * the bit to clear.
   * @return A reference to this instance for chaining.
   */
  template<bit_mask field>
  constexpr auto& clear()
  {
    static_assert(field.position < width,
                  "Bit position exceeds register width");
    constexpr auto inverted_mask = ~field.value<T>();

    m_value = m_value & inverted_mask;

    return *this;
  }

  /**
   * @brief Clears (sets to 0) multiple bits in the represented value.
   *
   * @param p_field A bit_mask instance, which represents the position and size
   * of the bit to clear.
   * @return A reference to this instance for chaining.
   */
  constexpr auto& clear(bit_mask p_field)
  {
    auto const inverted_mask = ~p_field.value<T>();

    m_value = m_value & inverted_mask;

    return *this;
  }

  /**
   * @brief Toggles multiple bits in the represented value.
   *
   * @tparam field A bit_mask type, which represents the position and size of
   * the bit to toggle.
   * @return A reference to this instance for chaining.
   */
  template<bit_mask field>
  constexpr auto& toggle()
  {
    static_assert(field.position < width,
                  "Bit position exceeds register width");

    constexpr auto mask = field.value<T>();

    m_value = m_value ^ mask;

    return *this;
  }

  /**
   * @brief Toggles a single bit in the represented value.
   *
   * @param p_field A bit_mask instance, which represents the position and size
   * of the bit to toggle.
   * @return A reference to this instance for chaining.
   */
  constexpr auto& toggle(bit_mask p_field)
  {
    auto const mask = p_field.value<T>();

    m_value = m_value ^ mask;

    return *this;
  }

  /**
   * @brief Inserts a new value into the represented bit sequence.
   *
   * @tparam field A bit_mask type, which represents the position and size of
   * the bit to insert.
   * @param p_value The new value to insert.
   * @return A reference to this instance for chaining.
   */
  template<bit_mask field>
  constexpr auto& insert(std::unsigned_integral auto p_value)
  {
    auto const value_to_insert = static_cast<T>(p_value);
    // AND value with mask to remove any bits beyond the specified width.
    // Shift masked value into bit position and OR with target value.
    auto const shifted_field = value_to_insert << field.position;
    auto const new_value = shifted_field & field.value<T>();

    // Clear width's number of bits in the target value at the bit position
    // specified.
    m_value = m_value & ~field.value<T>();
    m_value = m_value | static_cast<T>(new_value);

    return *this;
  }

  /**
   * @brief Inserts a new value into the represented bit sequence.
   *
   * @param p_field A bit_mask instance, which represents the position and size
   * of the bit to insert.
   * @param p_value The new value to insert.
   * @return A reference to this instance for chaining.
   */
  constexpr auto& insert(bit_mask p_field, std::unsigned_integral auto p_value)
  {
    // AND value with mask to remove any bits beyond the specified width.
    // Shift masked value into bit position and OR with target value.
    auto shifted_field = static_cast<T>(p_value) << p_field.position;
    auto new_value = shifted_field & p_field.value<T>();

    // Clear width's number of bits in the target value at the bit position
    // specified.
    m_value = m_value & ~p_field.value<T>();
    m_value = m_value | static_cast<T>(new_value);

    return *this;
  }

  /**
   * @brief Inserts a new value into the represented bit sequence.
   *
   * @tparam p_field A bit_mask instance, which represents the position and size
   * of the bit to insert.
   * @tparam p_value The new value to insert.
   * @return A reference to this instance for chaining.
   */
  template<bit_mask p_field, std::unsigned_integral auto p_value>
  constexpr auto& insert()
  {
    // AND value with mask to remove any bits beyond the specified width.
    // Shift masked value into bit position and OR with target value.
    auto shifted_field = static_cast<T>(p_value) << p_field.position;
    auto new_value = shifted_field & p_field.value<T>();

    // Clear width's number of bits in the target value at the bit position
    // specified.
    m_value = m_value & ~p_field.value<T>();
    m_value = m_value | static_cast<T>(new_value);

    return *this;
  }

  /**
   * @brief Returns the represented value as an instance of U
   *
   * Performs truncating static cast if the sizeof(T) > sizeof(U)
   *
   * @return The represented value as type U.
   */
  template<std::integral U>
  [[nodiscard]] constexpr auto to()
  {
    return static_cast<U>(m_value);
  }

  /**
   * @brief Returns the represented value as a T instance.
   *
   * @return The represented value.
   */
  [[nodiscard]] constexpr T get()
  {
    return m_value;
  }

protected:
  T m_value;
};

/**
 * @ingroup Bit
 * @brief A class for modifying a register value by manipulating its bits.
 *
 * This class provides a convenient and expressive way to perform bitwise
 * operations on a register value, allowing for efficient and safe modifications
 * without compromising performance.
 *
 * USAGE:
 *
 *     static constexpr auto pre_scalar = hal::bit_mask::from<3, 18>();
 *     static constexpr auto enable = hal::bit_mask::from<19>();
 *
 *     hal::bit_modify(reg->control)
 *        .insert<pre_scalar>(120UL)
 *        .set<enable>();
 *     // At this line, reg->control will be updated
 *
 * @tparam T - defaults to `std::uint32_t`, but can be any unsigned integral
 * type.
 */
template<std::unsigned_integral T>
class bit_modify : public bit_value<T>
{
public:
  /**
   * @brief Constructs a new bit_modify instance with an initial value and a
   * pointer to the register.
   *
   * @param p_register_reference A reference to the register whose bits will be
   * modified.
   */
  constexpr bit_modify(T volatile& p_register_reference)
    : bit_value<T>(p_register_reference)
    , m_pointer(&p_register_reference)
  {
  }

  /**
   * @brief On destruction, update the original register's value with the final
   * modified bits.
   *
   */
  ~bit_modify()
  {
    *m_pointer = this->m_value;
  }

private:
  /**
   * @brief A pointer to the original register value being modified.
   *
   * This allows for safe and efficient updates of the underlying register with
   * the final modified bits.
   */
  T volatile* m_pointer;
};
}  // namespace hal
