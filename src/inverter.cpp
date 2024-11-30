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

#include <libhal-util/inverter.hpp>

namespace hal {
output_pin_inverter::output_pin_inverter(hal::output_pin& p_output_pin)
  : m_output_pin(&p_output_pin)
{
}

input_pin_inverter::input_pin_inverter(hal::input_pin& p_input_pin)
  : m_input_pin(&p_input_pin)
{
}

void output_pin_inverter::driver_configure(
  hal::output_pin::settings const& p_settings)
{
  m_output_pin->configure(p_settings);
}

void output_pin_inverter::driver_level(bool p_high)
{
  m_output_pin->level(!p_high);
}

bool output_pin_inverter::driver_level()
{
  return !m_output_pin->level();
}

void input_pin_inverter::driver_configure(
  hal::input_pin::settings const& p_settings)
{
  m_input_pin->configure(p_settings);
}

bool input_pin_inverter::driver_level()
{
  return !m_input_pin->level();
}
}  // namespace hal
