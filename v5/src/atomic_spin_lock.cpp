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

#include <libhal-util/atomic_spin_lock.hpp>

#include <libhal-util/steady_clock.hpp>

namespace hal {
void atomic_spin_lock::os_lock()
{
  // In order to acquire a lock, the lock must first be available. In the case
  // of atomic_flag, the lock is available, or unlocked, if it is set to false.
  // The flag is considered locked if its value is set to true.
  // So if the current thread calls `test_and_set()`, then it sets the flag to
  // true. If the value returned from this was false, it means that the flag was
  // originally available and this thread was the one that locked it by setting
  // the value to true. If the return value was true, then that indicates that
  // the lock was not available and the thread must wait.
  while (m_flag.test_and_set(std::memory_order_acquire)) {
    continue;  // spin lock
  }
}

void atomic_spin_lock::os_unlock()
{
  m_flag.clear();
}

bool atomic_spin_lock::os_try_lock()
{
  // We invert this because we actually acquire the lock when previous state of
  // the lock was false. Seeing the flag as false meant that it was available.
  // if we attempted to take the lock and it was originally true, then the lock
  // was not available and the code needs to keep polling it until it receives a
  // false back.
  return not m_flag.test_and_set(std::memory_order_acquire);
}

void timed_atomic_spin_lock::os_lock()
{
  m_atomic_spin_lock.lock();
}

void timed_atomic_spin_lock::os_unlock()
{
  m_atomic_spin_lock.unlock();
}

bool timed_atomic_spin_lock::os_try_lock()
{
  return m_atomic_spin_lock.try_lock();
}

bool timed_atomic_spin_lock::os_try_lock_for(hal::time_duration p_poll_time)
{
  auto future_deadline = hal::future_deadline(*m_steady_clock, p_poll_time);

  while (m_steady_clock->uptime() < future_deadline) {
    auto acquired_lock = m_atomic_spin_lock.try_lock();
    if (acquired_lock) {
      return true;
    }
  }

  return false;
}
}  // namespace hal
