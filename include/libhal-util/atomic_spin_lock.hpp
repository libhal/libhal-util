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

#include <atomic>

#include <libhal/lock.hpp>
#include <libhal/steady_clock.hpp>

namespace hal::soft {
/**
 * @brief Atomic spin lock that implements hal::pollable_lock
 *
 * This lock provides an operating system agnostic lock that works on any
 * processor that support lock-free atomic boolean operations, which should be
 * almost all systems.
 *
 * This lock will perform a spin lock until it acquires a lock. Using such a
 * lock on a properly multithreaded system is inefficient as it does not have
 * the capability to notify the system that a thread is currently waiting for
 * the lock to be made available.
 *
 * This lock is useful as a default lock for platform libraries, in order to
 * ensure thread safety. Such platforms should provide an API for swapping out
 * the atomic spin lock with an appropriate lock provided by the users operating
 * system.
 */
class atomic_spin_lock : public hal::pollable_lock
{
public:
  /**
   * @brief Construct a new atomic spin lock object
   *
   */
  atomic_spin_lock() = default;
  ~atomic_spin_lock() = default;

private:
  void os_lock() override;
  void os_unlock() override;
  bool os_try_lock() override;

  std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

/**
 * @brief Same as hal::atomic_spin_lock but supports timed_lock apis
 *
 * All of the dubious usages of hal::atomic_spin_lock follow with this lock as
 * well. In general, do not use this in production. Use the appropriate thread
 * safe lock for your operating system.
 */
class timed_atomic_spin_lock : public hal::timed_lock
{
public:
  /**
   * @brief Construct a new timed atomic spin lock object
   *
   * @param p_steady_clock - steady clock used to time the try_lock_for api.
   */
  timed_atomic_spin_lock(hal::steady_clock& p_steady_clock)
    : m_steady_clock(&p_steady_clock)
  {
  }

  ~timed_atomic_spin_lock() = default;

private:
  void os_lock() override;
  void os_unlock() override;
  bool os_try_lock() override;
  bool os_try_lock_for(hal::time_duration p_poll_time) override;

  hal::steady_clock* m_steady_clock;
  atomic_spin_lock m_atomic_spin_lock;
};
}  // namespace hal::soft
