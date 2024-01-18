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

#include <libhal-util/static_callable.hpp>

#include <boost/ut.hpp>

namespace hal {
void static_callable_test()
{
  using namespace boost::ut;

  // Setup
  class dummy_driver
  {};

  "static_callable void(void)"_test = []() {
    // Setup
    using callback1_signature = void (*)();
    bool callback1_was_called = false;

    auto callable1 = static_callable<dummy_driver, 1, void(void)>(
      [&callback1_was_called]() { callback1_was_called = true; });
    callback1_signature callback1 = callable1.get_handler();

    // Exercise & Verify
    expect(that % false == callback1_was_called);
    callback1();
    expect(that % true == callback1_was_called);
  };

  "static_callable void(bool)"_test = []() {
    // Setup
    using callback2_signature = void (*)(bool);
    bool callback2_was_called = false;
    bool captured_bool = false;

    auto callable2 = static_callable<dummy_driver, 2, void(bool)>(
      [&callback2_was_called, &captured_bool](bool value) {
        callback2_was_called = true;
        captured_bool = value;
      });

    callback2_signature callback2 = callable2.get_handler();

    // Exercise & Verify
    expect(that % false == captured_bool);
    callback2(true);
    expect(that % true == callback2_was_called);
    expect(that % true == captured_bool);
    callback2(false);
    expect(that % false == captured_bool);
  };
};
}  // namespace hal
