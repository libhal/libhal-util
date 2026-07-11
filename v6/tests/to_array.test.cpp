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

#include <boost/ut.hpp>

import hal.util;

namespace {

void to_array_test()
{
  using namespace boost::ut;

  "hal::to_array()"_test = [] {
    "exact fit"_test = [] {
      auto const result = hal::to_array<5>("hello");
      expect(that % 6 == result.size());
      expect(that % 'h' == result[0]);
      expect(that % 'e' == result[1]);
      expect(that % 'l' == result[2]);
      expect(that % 'l' == result[3]);
      expect(that % 'o' == result[4]);
      expect(that % '\0' == result[5]);
    };
    "truncates when view is longer than N"_test = [] {
      auto const result = hal::to_array<3>("hello");
      expect(that % 4 == result.size());
      expect(that % 'h' == result[0]);
      expect(that % 'e' == result[1]);
      expect(that % 'l' == result[2]);
      expect(that % '\0' == result[3]);
    };
    "zero fills remainder when view is shorter than N"_test = [] {
      auto const result = hal::to_array<5>("hi");
      expect(that % 6 == result.size());
      expect(that % 'h' == result[0]);
      expect(that % 'i' == result[1]);
      expect(that % '\0' == result[2]);
      expect(that % '\0' == result[3]);
      expect(that % '\0' == result[4]);
      expect(that % '\0' == result[5]);
    };
    "empty view"_test = [] {
      auto const result = hal::to_array<4>("");
      expect(that % 5 == result.size());
      expect(that % '\0' == result[0]);
      expect(that % '\0' == result[1]);
      expect(that % '\0' == result[2]);
      expect(that % '\0' == result[3]);
      expect(that % '\0' == result[4]);
    };
    "zero size array"_test = [] {
      auto const result = hal::to_array<0>("");
      expect(that % 1 == result.size());
      expect(that % '\0' == result[0]);
    };
  };
}

}  // namespace

int main()
{
  to_array_test();
}
