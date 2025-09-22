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

#pragma once

#include <concepts>
#include <functional>
#include <ios>
#include <system_error>

#include <libhal/error.hpp>
#include <libhal/timeout.hpp>

#include "units.hpp"

/**
 * @defgroup TimeoutUtil Timeout Util
 *
 */

namespace hal {
constexpr std::string_view to_string(work_state p_state)
{
  switch (p_state) {
    case work_state::in_progress:
      return "in progress";
    case work_state::failed:
      return "failed";
    case work_state::finished:
      return "finished";
    default:
      return "unknown work state";
  }
}

constexpr bool terminated(work_state p_state)
{
  return p_state == work_state::finished || p_state == work_state::failed;
}

constexpr bool finished(work_state p_state)
{
  return p_state == work_state::finished;
}

constexpr bool in_progress(work_state p_state)
{
  return p_state == work_state::in_progress;
}

constexpr bool failed(work_state p_state)
{
  return p_state == work_state::failed;
}
// Turning off clang-format because concepts because give it an aneurysm.
// clang-format off
template<typename T>
concept has_work_state = requires(T a) {
                           {
                             a.state()
                             } -> std::same_as<work_state>;
                         };
// clang-format on

constexpr bool terminated(has_work_state auto p_worker)
{
  return terminated(p_worker.state());
}

constexpr bool finished(has_work_state auto p_worker)
{
  return finished(p_worker.state());
}

constexpr bool in_progress(has_work_state auto p_worker)
{
  return in_progress(p_worker.state());
}

constexpr bool failed(has_work_state auto p_worker)
{
  return failed(p_worker.state());
}

template<class CharT, class Traits>
inline std::basic_ostream<CharT, Traits>& operator<<(
  std::basic_ostream<CharT, Traits>& p_ostream,
  work_state const& p_state)
{
  return p_ostream << to_string(p_state);
}

/**
 * @ingroup TimeoutUtil
 * @brief Repeatedly call a worker function until it has reached a terminal
 * state or a timeout has been reached
 *
 * @param p_worker - worker function to repeatedly call
 * @param p_timeout - callable timeout object
 * @return work_state - state of the worker function
 */
inline work_state try_until(worker auto& p_worker, timeout auto p_timeout)
{
  while (true) {
    p_timeout();
    auto state = p_worker();
    if (hal::terminated(state)) {
      return state;
    }
  }
};

/**
 * @ingroup TimeoutUtil
 * @brief Repeatedly call a worker function until it has reached a terminal
 * state or a timeout has been reached
 *
 * @param p_worker - worker function to repeatedly call
 * @param p_timeout - callable timeout object
 * @return work_state - state of the worker function
 */
inline work_state try_until(worker auto&& p_worker, timeout auto p_timeout)
{
  worker auto& worker = p_worker;
  return try_until(worker, p_timeout);
};
}  // namespace hal
