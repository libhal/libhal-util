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

// edge_triggered_interrupt_dispatcher misuse that the type system rejects.

#include "fixtures.hpp"

using dispatcher = hal::edge_triggered_interrupt_dispatcher<2>;

// A slot of a temporary dispatcher would dangle immediately.
void index_temporary_dispatcher(fake_pin& p_pin)
{
  // expected-error@+1 {{overload resolution selected deleted operator '[]'}}
  auto& slot = dispatcher(p_pin)[0];
  slot.configure({});
}

// The dispatcher must not outlive its pin.
void dispatcher_on_temporary_pin()
{
  // expected-error@+1 {{no matching constructor}}
  dispatcher shared(fake_pin{});
}

// Slots are registered with the pin through the dispatcher's address.
void copy_dispatcher(fake_pin& p_pin)
{
  dispatcher shared(p_pin);
  // expected-error@+1 {{call to deleted constructor}}
  dispatcher copy = shared;
}
