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

#include <atomic>

export module hal.util:busy_scope;

import hal;

/**
 * @defgroup BusyScope Busy Scope
 *
 */

namespace hal::inline v6 {
/**
 * @ingroup BusyScope
 * @brief Marks a driver busy for its lifetime, so that an overlapping call
 * throws instead of interfering
 *
 * Construct one at the start of an operation that must not overlap with
 * another on the same driver, such as a transaction started from an interrupt
 * while another context's transaction is running. The destructor clears the
 * mark, even if the operation throws. To make callers wait instead, use a
 * lock.
 *
 * Only use it on platforms with atomic support, where `std::atomic_flag`
 * compiles to atomic instructions. On a core without them, such as a
 * Cortex-M0+, `test_and_set()` becomes a library call that the toolchain may
 * not provide, and linking fails. So platform libraries, which know their
 * target, can use it, and device libraries, which must build for every target,
 * can't.
 */
export class busy_scope
{
public:
  // Members are defined inline, so that only a library that uses busy_scope
  // compiles its atomic operations, and libhal-util builds for every target.

  /**
   * @brief Mark the driver busy
   *
   * @param p_busy - the driver's busy flag
   * @param p_instance - the driver, reported by the exception
   * @throws hal::device_or_resource_busy - if `p_busy` is already set: another
   * operation on the driver is in progress
   */
  inline busy_scope(std::atomic_flag& p_busy [[clang::lifetimebound]],
                    void const* p_instance)
    : m_busy(p_busy)
  {
    if (m_busy.test_and_set(std::memory_order_acquire)) {
      throw hal::device_or_resource_busy(p_instance);
    }
  }

  busy_scope(busy_scope const&) = delete;
  busy_scope& operator=(busy_scope const&) = delete;
  busy_scope(busy_scope&&) = delete;
  busy_scope& operator=(busy_scope&&) = delete;

  /**
   * @brief Clear the driver's busy flag
   *
   */
  inline ~busy_scope()
  {
    m_busy.clear(std::memory_order_release);
  }

private:
  std::atomic_flag& m_busy;
};
}  // namespace hal::inline v6
