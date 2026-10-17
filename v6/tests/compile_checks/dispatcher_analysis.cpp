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

// edge_triggered_interrupt_dispatcher misuse that Clang's lifetime safety
// analysis reports.

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
