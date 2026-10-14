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

#include <optional>
#include <vector>

#include <boost/ut.hpp>

import hal;
import hal.util;

namespace {
struct fake_pin final : hal::edge_triggered_interrupt
{
  void trigger(bool p_state)
  {
    if (callback != nullptr) {
      callback->callback(p_state);
    }
  }

  hal::edge_triggered_callback* callback = nullptr;
  std::optional<hal::edge_trigger> trigger_setting;

private:
  void driver_configure(settings const& p_settings) override
  {
    trigger_setting = p_settings.trigger;
  }
  void driver_on_trigger(hal::edge_triggered_callback& p_callback) override
  {
    if (callback != nullptr) {
      throw hal::subscription_busy(this);
    }
    callback = &p_callback;
  }
  void driver_clear_trigger() noexcept override
  {
    callback = nullptr;
  }
};

struct recorder final : hal::edge_triggered_callback
{
  void callback(bool p_state) noexcept override
  {
    states.push_back(p_state);
  }
  std::vector<bool> states;
};

void edge_triggered_interrupt_dispatcher_test()
{
  using namespace boost::ut;

  "subscribes to the pin and configures both edges"_test = []() {
    // Setup
    fake_pin pin;

    // Exercise
    {
      hal::edge_triggered_interrupt_dispatcher<2> dispatcher(pin);

      // Verify
      expect(pin.callback != nullptr);
      expect(pin.trigger_setting == hal::edge_trigger::both);
    }

    // Verify
    expect(pin.callback == nullptr) << "destroying it unsubscribes";
  };

  "throws when the pin already has a subscriber"_test = []() {
    // Setup
    fake_pin pin;
    recorder other;
    auto subscription = pin.on_trigger(other);

    // Exercise + Verify
    expect(throws<hal::subscription_busy>(
      [&]() { hal::edge_triggered_interrupt_dispatcher<2> dispatcher(pin); }));
  };

  "forwards each edge to the slots whose trigger matches"_test = []() {
    // Setup
    fake_pin pin;
    hal::edge_triggered_interrupt_dispatcher<3> dispatcher(pin);
    recorder rising;
    recorder falling;
    recorder both;
    dispatcher[1].configure({ .trigger = hal::edge_trigger::falling });
    dispatcher[2].configure({ .trigger = hal::edge_trigger::both });
    auto rising_subscription = dispatcher[0].on_trigger(rising);
    auto falling_subscription = dispatcher[1].on_trigger(falling);
    auto both_subscription = dispatcher[2].on_trigger(both);

    // Exercise
    pin.trigger(true);
    pin.trigger(false);
    pin.trigger(true);

    // Verify
    expect(that % rising.states == std::vector{ true, true });
    expect(that % falling.states == std::vector{ false });
    expect(that % both.states == std::vector{ true, false, true });
  };

  "configuring a slot does not change the pin"_test = []() {
    // Setup
    fake_pin pin;
    hal::edge_triggered_interrupt_dispatcher<1> dispatcher(pin);

    // Exercise
    dispatcher[0].configure({ .trigger = hal::edge_trigger::falling });

    // Verify
    expect(pin.trigger_setting == hal::edge_trigger::both);
  };

  "a slot accepts one subscriber at a time"_test = []() {
    // Setup
    fake_pin pin;
    hal::edge_triggered_interrupt_dispatcher<1> dispatcher(pin);
    recorder first;
    recorder second;

    // Exercise + Verify
    {
      auto subscription = dispatcher[0].on_trigger(first);
      expect(throws<hal::subscription_busy>(
        [&]() { auto busy = dispatcher[0].on_trigger(second); }));
    }
    auto subscription = dispatcher[0].on_trigger(second);
    pin.trigger(true);
    expect(that % first.states.size() == 0);
    expect(that % second.states == std::vector{ true });
  };

  "an unsubscribed slot is skipped"_test = []() {
    // Setup
    fake_pin pin;
    hal::edge_triggered_interrupt_dispatcher<2> dispatcher(pin);
    recorder second;
    auto subscription = dispatcher[1].on_trigger(second);

    // Exercise
    pin.trigger(true);

    // Verify
    expect(that % second.states == std::vector{ true });
  };

  "operator[] throws out_of_range past the last slot"_test = []() {
    // Setup
    fake_pin pin;
    hal::edge_triggered_interrupt_dispatcher<2> dispatcher(pin);

    // Exercise + Verify
    expect(throws<hal::out_of_range>([&]() { (void)dispatcher[2]; }));
    expect(that % dispatcher.size() == 2);
  };
}
}  // namespace

int main()
{
  edge_triggered_interrupt_dispatcher_test();
}
