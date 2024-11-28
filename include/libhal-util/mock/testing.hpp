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

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <ostream>
#include <span>
#include <tuple>
#include <vector>

#include <libhal/error.hpp>

namespace hal {
/**
 * @brief Helper utility for making mocks for class functions that return
 * status.
 *
 * This class stores records of a functions call history in order to be
 * recovered later for inspection in tests and simulations.
 *
 * See pwm_mock.hpp and tests/pwm_mock.test.cpp as an example of how this is
 * done in practice.
 *
 * @tparam args_t - the arguments of the class function
 */
template<typename... args_t>
class spy_handler
{
public:
  /**
   * @brief Set the record function to return an error after a specified number
   * of recordings.
   *
   * @param p_call_count_before_trigger - the number of calls before an error
   * is thrown.
   * @param p_exception_callback - a callable function that throws an exception
   * when p_call_count_before_trigger reaches 1.
   * @throws std::range_error - if p_call_count_before_trigger is below 0.
   */
  template<typename F>
  void trigger_error_on_call(int p_call_count_before_trigger,
                             F&& p_exception_callback)
  {
    if (p_call_count_before_trigger < 0) {
      throw std::range_error("trigger_error_on_call() must be 0 or above");
    }
    m_error_trigger = p_call_count_before_trigger;
    m_exception_callback = p_exception_callback;
  }

  /**
   * @brief Record the arguments of a function being spied on.
   *
   * @param p_args - arguments to record
   * @throws ? - once the error trigger count reaches 1. The error depends on
   * what is thrown from `p_exception_callback` in the `trigger_error_on_call`.
   */
  void record(args_t... p_args)
  {
    m_call_history.push_back(std::make_tuple(p_args...));

    if (m_error_trigger > 1) {
      m_error_trigger--;
    } else if (m_error_trigger == 1) {
      m_error_trigger--;
      if (m_exception_callback) {
        m_exception_callback();
      }
    }
  }

  /**
   * @brief Return the call history of the save function
   *
   * @return const auto& - reference to the call history vector
   */
  [[nodiscard]] auto const& call_history() const
  {
    return m_call_history;
  }

  /**
   * @brief Return argument from one of call history parameters
   *
   * @param p_call - history call from 0 to N
   * @return const auto& - reference to the call history vector
   * @throws std::out_of_range - if p_call is beyond the size of call_history
   */
  template<size_t ArgumentIndex>
  [[nodiscard]] auto const& history(size_t p_call) const
  {
    return std::get<ArgumentIndex>(m_call_history.at(p_call));
  }

  /**
   * @brief Reset call recordings and turns off error trigger
   *
   */
  void reset()
  {
    m_call_history.clear();
    m_error_trigger = 0;
  }

private:
  std::vector<std::tuple<args_t...>> m_call_history{};
  std::function<void()> m_exception_callback{};
  int m_error_trigger = 0;
};
}  // namespace hal

template<typename Rep, typename Period>
inline std::ostream& operator<<(
  std::ostream& p_os,
  std::chrono::duration<Rep, Period> const& p_duration)
{
  return p_os << p_duration.count() << " * (" << Period::num << "/"
              << Period::den << ")s";
}

template<typename T, size_t size>
inline std::ostream& operator<<(std::ostream& p_os,
                                std::array<T, size> const& p_array)
{
  p_os << "{";
  for (auto const& element : p_array) {
    p_os << element << ", ";
  }
  return p_os << "}\n";
}

template<typename T>
inline std::ostream& operator<<(std::ostream& p_os, std::span<T> const& p_array)
{
  p_os << "{";
  for (auto const& element : p_array) {
    p_os << element << ", ";
  }
  return p_os << "}\n";
}
