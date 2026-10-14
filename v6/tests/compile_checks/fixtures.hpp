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

// Types shared by the compile checks (see libhal_add_compile_checks() in
// libhal-cmake-util).

#pragma once

import hal;
import hal.util;

struct fake_pin final : hal::edge_triggered_interrupt
{
  hal::edge_triggered_callback* callback = nullptr;

private:
  void driver_configure(settings const&) override
  {
  }
  void driver_on_trigger(hal::edge_triggered_callback& p_callback) override
  {
    if (callback) {
      throw hal::subscription_busy(this);
    }
    callback = &p_callback;
  }
  void driver_clear_trigger() noexcept override
  {
    callback = nullptr;
  }
};

struct counter final : hal::edge_triggered_callback
{
  void callback(bool) noexcept override
  {
  }
};

struct fake_can_interrupt final : hal::can_interrupt
{
  hal::can_receive_callback* callback = nullptr;

private:
  void driver_on_receive(hal::can_receive_callback& p_callback) override
  {
    if (callback) {
      throw hal::subscription_busy(this);
    }
    callback = &p_callback;
  }
  void driver_clear_receive() noexcept override
  {
    callback = nullptr;
  }
};

struct message_counter final : hal::can_receive_callback
{
  void callback(hal::can_message const&) noexcept override
  {
  }
};
