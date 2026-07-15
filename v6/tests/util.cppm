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

#include <ios>

export module test_util;

import hal.util;
import async_context;
import scatter_span;

namespace mem {
/* Prints the logical element sequence of a scatter_span, e.g. `[1, 2, 3]`.
 * Lets boost::ut print the operands of a failed `expect` involving a
 * scatter_span.
 */
export template<typename T>
std::ostream& operator<<(std::ostream& p_os, scatter_span<T> const& p_ssp)
{
  p_os << "[";
  for (auto const& chunk : p_ssp) {
    auto const* addr = static_cast<void const*>(chunk.data());
    p_os << "(" << addr << " : " << chunk.size() << "), ";
  }
  p_os << "]";
  return p_os;
}

/* A helper function to compare two scatter_spans. This is only useful for
 * testing
 */
export template<typename T>
bool operator==(scatter_span<T> const& lhs, scatter_span<T> const& rhs)
{

  if (lhs.length() != rhs.length()) {
    return false;
  }

  if (lhs.length() == 0) {
    return true;
  }

  auto r_iter = rhs.begin();
  auto l_iter = lhs.begin();

  for (; r_iter != rhs.end() and l_iter != lhs.end(); r_iter++, l_iter++) {
    if (r_iter->data() != l_iter->data() and r_iter->size() != l_iter->size()) {
      return false;
    }
  }

  return true;
}
}  // namespace mem

export template<typename T>
void finish(async::future<T>& p_future)
{
  while (not p_future.done()) {
    p_future.resume();
  }
}
