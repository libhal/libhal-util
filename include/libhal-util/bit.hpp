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
  constexpr bool operator==(const bit_mask& other)
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

template<bit_mask field>
constexpr auto bit_extract(std::unsigned_integral auto p_value)
{
  using T = decltype(p_value);
  // Shift desired value to the right to position 0
  const auto shifted = p_value >> field.position;
  // Mask away any bits left of the value based on the field width
  const auto masked = shifted & field.origin<T>();
  // Leaving only the desired bits
  return static_cast<T>(masked);
}

constexpr auto bit_extract(bit_mask p_field,
                           std::unsigned_integral auto p_value)
{
  using T = decltype(p_value);
  // Shift desired value to the right to position 0
  const auto shifted = p_value >> p_field.position;
  // Mask away any bits left of the value based on the field width
  const auto masked = shifted & p_field.origin<T>();
  // Leaving only the desired bits
  return static_cast<T>(masked);
}

template<std::unsigned_integral T>
class bit_value
{
public:
  static constexpr std::uint32_t width = sizeof(T) * 8;

  constexpr bit_value(T p_initial_value = 0)
    : m_value(p_initial_value)
  {
  }

  template<bit_mask field>
  constexpr auto& set()
  {
    static_assert(field.position < width,
                  "Bit position exceeds register width");
    constexpr auto mask = static_cast<T>(1U << field.position);

    m_value = m_value | mask;

    return *this;
  }

  constexpr auto& set(bit_mask p_field)
  {
    const auto mask = static_cast<T>(1U << p_field.position);

    m_value = m_value | mask;

    return *this;
  }

  template<bit_mask field>
  constexpr auto& clear()
  {
    static_assert(field.position < width,
                  "Bit position exceeds register width");
    constexpr auto mask = static_cast<T>(1U << field.position);
    constexpr auto inverted_mask = ~mask;

    m_value = m_value & inverted_mask;

    return *this;
  }

  constexpr auto& clear(bit_mask p_field)
  {
    const auto mask = static_cast<T>(1U << p_field.position);
    const auto inverted_mask = ~mask;

    m_value = m_value & inverted_mask;

    return *this;
  }

  template<bit_mask field>
  constexpr auto& toggle()
  {
    static_assert(field.position < width,
                  "Bit position exceeds register width");

    constexpr auto mask = static_cast<T>(1U << field.position);

    m_value = m_value ^ mask;

    return *this;
  }

  constexpr auto& toggle(bit_mask p_field)
  {
    const auto mask = static_cast<T>(1U << p_field.position);

    m_value = m_value ^ mask;

    return *this;
  }

  template<bit_mask field>
  constexpr auto& insert(std::unsigned_integral auto p_value)
  {
    const auto value_to_insert = static_cast<T>(p_value);
    // AND value with mask to remove any bits beyond the specified width.
    // Shift masked value into bit position and OR with target value.
    const auto shifted_field = value_to_insert << field.position;
    const auto new_value = shifted_field & field.value<T>();

    // Clear width's number of bits in the target value at the bit position
    // specified.
    m_value = m_value & ~field.value<T>();
    m_value = m_value | static_cast<T>(new_value);

    return *this;
  }

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

  template<std::integral U>
  [[nodiscard]] constexpr auto to()
  {
    return static_cast<U>(m_value);
  }

  [[nodiscard]] constexpr T get()
  {
    return m_value;
  }

protected:
  T m_value;
};

template<std::unsigned_integral T>
class bit_modify : public bit_value<T>
{
public:
  constexpr bit_modify(volatile T& p_register_reference)
    : bit_value<T>(p_register_reference)
    , m_pointer(&p_register_reference)
  {
  }

  ~bit_modify()
  {
    *m_pointer = this->m_value;
  }

private:
  volatile T* m_pointer;
};
}  // namespace hal
