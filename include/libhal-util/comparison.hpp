#pragma once

#include <algorithm>
#include <array>
#include <span>

template<typename T, size_t size>
constexpr bool operator==(const std::array<T, size>& p_array,
                          const std::span<T>& p_span)
{
  if (p_span.size() != size) {
    return false;
  }

  return std::equal(p_array.begin(), p_array.end(), p_span.begin());
}

template<typename T, size_t size>
constexpr bool operator==(const std::span<T>& p_span,
                          const std::array<T, size>& p_array)
{
  return p_array == p_span;
}

template<typename T, size_t size>
constexpr bool operator!=(const std::array<T, size>& p_array,
                          const std::span<T>& p_span)
{
  return !(p_array == p_span);
}

template<typename T, size_t size>
constexpr bool operator!=(const std::span<T>& p_span,
                          const std::array<T, size>& p_array)
{
  return !(p_array == p_span);
}
