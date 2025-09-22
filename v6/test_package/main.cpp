// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <print>
#include <string_view>

import hal.util;

// #include <libhal-util/serial.hpp>
// #include <libhal-util/to_array.hpp>

// class stub_serial : public hal::serial
// {
// public:
//   void driver_configure(settings const&) override
//   {
//   }

//   write_t driver_write(std::span<hal::byte const> p_data) override
//   {
//     return write_t{ .data = p_data.subspan(0, 0) };
//   }

//   read_t driver_read(std::span<hal::byte> p_data) override
//   {
//     return read_t{ .data = p_data.subspan(0, 0),
//                    .available = 0,
//                    .capacity = 0 };
//   }

//   void driver_flush() override
//   {
//   }

//   ~stub_serial() override = default;
// };

int main()
{
  using namespace std::string_view_literals;

  auto array = hal::to_array<5>("Hello World!\n"sv);

  std::println("{}", array);
  // stub_serial stub;
  // hal::print(stub, "Goodbye, World!\n");

  return 0;
}
