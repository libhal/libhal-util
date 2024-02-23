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

namespace hal {
extern void as_bytes_test();
extern void bit_test();
extern void can_test();
extern void enum_test();
extern void i2c_util_test();
extern void input_pin_util_test();
extern void interrupt_pin_util_test();
extern void map_test();
extern void math_test();
extern void move_interceptor_test();
extern void output_pin_util_test();
extern void overflow_counter_test();
extern void serial_util_test();
extern void spi_util_test();
extern void static_callable_test();
extern void static_list_test();
extern void steady_clock_utility_test();
extern void timeout_test();
extern void units_test();
extern void stream_terminated_test();
extern void parse_stream_test();
extern void find_stream_test();
extern void fill_upto_stream_test();
extern void multi_stream_test();
}  // namespace hal

int main()
{
  hal::as_bytes_test();
  hal::bit_test();
  hal::can_test();
  hal::enum_test();
  hal::i2c_util_test();
  hal::input_pin_util_test();
  hal::interrupt_pin_util_test();
  hal::map_test();
  hal::math_test();
  hal::move_interceptor_test();
  hal::output_pin_util_test();
  hal::overflow_counter_test();
  hal::serial_util_test();
  hal::spi_util_test();
  hal::static_callable_test();
  hal::static_list_test();
  hal::steady_clock_utility_test();
  hal::timeout_test();
  hal::units_test();
  hal::stream_terminated_test();
  hal::parse_stream_test();
  hal::find_stream_test();
  hal::fill_upto_stream_test();
  hal::multi_stream_test();
}
