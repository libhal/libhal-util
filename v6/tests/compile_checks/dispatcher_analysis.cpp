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

// Interrupt dispatcher misuse that Clang's lifetime safety analysis reports.

#include "fixtures.hpp"

using dispatcher = hal::edge_triggered_interrupt_dispatcher<2>;

hal::edge_triggered_interrupt& slot_outlives_dispatcher(fake_pin& p_pin)
{
  dispatcher shared(p_pin);
  // expected-error@+2 {{is returned}}
  // expected-error@+1 {{reference to stack memory}}
  return shared[0];
}

hal::subscription subscription_outlives_dispatcher(fake_pin& p_pin)
{
  static counter c;
  dispatcher shared(p_pin);
  // expected-error@+2 {{is returned}}
  // expected-error@+1 {{address of stack memory}}
  return shared[0].on_trigger(c);
}

dispatcher const* dispatcher_outlives_pin()
{
  fake_pin pin;  // forgot static
  // expected-error@+1 {{which will dangle}}
  static dispatcher shared(pin);
  return &shared;
}

using can_dispatcher = hal::can_interrupt_dispatcher<2>;

hal::can_interrupt& can_slot_outlives_dispatcher(
  fake_can_interrupt& p_interrupt)
{
  can_dispatcher shared(p_interrupt);
  // expected-error@+2 {{is returned}}
  // expected-error@+1 {{reference to stack memory}}
  return shared[0];
}

hal::subscription can_subscription_outlives_dispatcher(
  fake_can_interrupt& p_interrupt)
{
  static message_counter c;
  can_dispatcher shared(p_interrupt);
  // expected-error@+2 {{is returned}}
  // expected-error@+1 {{address of stack memory}}
  return shared[0].on_receive(c);
}

can_dispatcher const* can_dispatcher_outlives_interrupt()
{
  fake_can_interrupt interrupt;  // forgot static
  // expected-error@+1 {{which will dangle}}
  static can_dispatcher shared(interrupt);
  return &shared;
}
