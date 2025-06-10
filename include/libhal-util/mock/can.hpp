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

#pragma once

#include <libhal/can.hpp>
#include <libhal/functional.hpp>

#include "testing.hpp"

namespace hal {
/**
 * @brief Mock can implementation for use in unit tests and simulations
 *
 */
struct mock_can : public hal::can
{
  /**
   * @brief Reset spy information for functions
   *
   */
  void reset()
  {
    spy_configure.reset();
    spy_send.reset();
    spy_on_receive.reset();
    spy_bus_on.reset();
  }

  /// Spy handler for hal::can::configure()
  spy_handler<settings> spy_configure;
  /// Spy handler for hal::can::send()
  spy_handler<message_t> spy_send;
  /// Spy handler for hal::can::bus_on() will always have content of "true"
  spy_handler<bool> spy_bus_on;
  /// Spy handler for hal::can::on_receive()
  spy_handler<hal::callback<handler>> spy_on_receive;

private:
  void driver_configure(settings const& p_settings) override
  {
    spy_configure.record(p_settings);
  }

  void driver_bus_on() override
  {
    spy_bus_on.record(true);
  }

  void driver_send(message_t const& p_message) override
  {
    spy_send.record(p_message);
  }

  void driver_on_receive(hal::callback<handler> p_handler) override
  {
    spy_on_receive.record(p_handler);
  }
};
}  // namespace hal

/**
 * @brief print can::message_t type using ostreams
 *
 * Meant for unit testing, testing and simulation purposes
 * C++ streams, in general, should not be used for any embedded project that
 * will ever have to be used on an MCU due to its memory cost.
 *
 * @tparam CharT - character type
 * @tparam Traits - ostream traits type
 * @param p_ostream - the ostream
 * @param p_message - object to convert to a string
 * @return std::basic_ostream<CharT, Traits>& - reference to the ostream
 */
template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
  std::basic_ostream<CharT, Traits>& p_ostream,
  hal::can::message_t const& p_message)
{
  p_ostream << "{ id: " << std::hex << "0x" << p_message.id;
  p_ostream << ", length: " << std::dec << unsigned{ p_message.length };
  p_ostream << ", is_remote_request: " << p_message.is_remote_request;
  p_ostream << ", payload = [";
  for (auto const& element : p_message.payload) {
    p_ostream << std::hex << "0x" << unsigned{ element } << ", ";
  }
  p_ostream << "] }";
  return p_ostream;
}
