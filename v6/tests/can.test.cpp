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

#include <array>
#include <coroutine>
#include <cstdint>
#include <optional>

#include <boost/ut.hpp>

import hal;
import hal.util;
import test_util;

using namespace mp_units::si::unit_symbols;

namespace {

void check_validity(hal::hertz p_operating_frequency,
                    hal::hertz p_target_baud_rate)
{
  using namespace boost::ut;

  auto call_result =
    hal::calculate_can_bus_divider(p_operating_frequency, p_target_baud_rate);

  expect(that % call_result.has_value());

  auto test_subject = call_result.value();  // NOLINT

  auto const bit_width = hal::bit_width(test_subject);

  auto const calculated_bit_width =
    (test_subject.sync_segment + test_subject.propagation_delay +
     test_subject.phase_segment1 + test_subject.phase_segment2);
  auto const denominator =
    static_cast<float>(test_subject.clock_divider * calculated_bit_width);
  auto const operating_frequency_value =
    p_operating_frequency.numerical_value_in(hal::hertz::unit);
  auto const target_baud_rate_value =
    p_target_baud_rate.numerical_value_in(hal::hertz::unit);
  auto const calculated_baud_rate = operating_frequency_value / denominator;

  expect(that % 8 <= bit_width && bit_width <= 25);
  expect(that % 8 <= test_subject.total_tq && test_subject.total_tq <= 25);
  expect(that % bit_width == test_subject.total_tq);
  expect(that % static_cast<std::uint32_t>(calculated_baud_rate) ==
         static_cast<std::uint32_t>(target_baud_rate_value));
}

class test_can_transceiver : public hal::can_transceiver
{
public:
  ~test_can_transceiver() override = default;

  void add_to_received_messages(hal::can_message const& p_message)
  {
    m_buffer[m_cursor++ % m_buffer.size()] = p_message;
  }

  hal::can_message m_last_sent_message{};
  std::array<hal::can_message, 12> m_buffer{};
  std::size_t m_cursor = 0;

private:
  async::future<hal::hertz> driver_baud_rate(async::context&) override
  {
    co_return 100'000 * Hz;
  }

  async::future<void> driver_send(async::context&,
                                  hal::can_message const& p_message) override
  {
    m_last_sent_message = p_message;
    return {};
  }

  hal::circular_span<hal::can_message const> driver_receive_buffer() override
  {
    return m_buffer;
  }

  hal::usize driver_receive_cursor() override
  {
    return m_cursor % m_buffer.size();
  }
};

async::inplace_context<1024> ctx;

void divider_test()
{
  using namespace boost::ut;

  "calculate_can_bus_divider(operation_frequency, desired_baud_rate)"_test =
    []() {
      check_validity(8'000'000 * Hz, 100'000 * Hz);
      check_validity(8'000'000 * Hz, 250'000 * Hz);
      check_validity(8'000'000 * Hz, 500'000 * Hz);
      check_validity(8'000'000 * Hz, 1'000'000 * Hz);

      check_validity(16'000'000 * Hz, 100'000 * Hz);
      check_validity(16'000'000 * Hz, 250'000 * Hz);
      check_validity(16'000'000 * Hz, 500'000 * Hz);
      check_validity(16'000'000 * Hz, 1'000'000 * Hz);

      check_validity(46'000'000 * Hz, 100'000 * Hz);
      check_validity(46'000'000 * Hz, 250'000 * Hz);
      check_validity(46'000'000 * Hz, 500'000 * Hz);
      check_validity(46'000'000 * Hz, 1'000'000 * Hz);

      check_validity(64'000'000 * Hz, 100'000 * Hz);
      check_validity(64'000'000 * Hz, 250'000 * Hz);
      check_validity(64'000'000 * Hz, 500'000 * Hz);
      check_validity(64'000'000 * Hz, 1'000'000 * Hz);

      check_validity(96'000'000 * Hz, 100'000 * Hz);
      check_validity(96'000'000 * Hz, 250'000 * Hz);
      check_validity(96'000'000 * Hz, 500'000 * Hz);
      check_validity(96'000'000 * Hz, 1'000'000 * Hz);

      check_validity(120'000'000 * Hz, 100'000 * Hz);
      check_validity(120'000'000 * Hz, 250'000 * Hz);
      check_validity(120'000'000 * Hz, 500'000 * Hz);
      check_validity(120'000'000 * Hz, 1'000'000 * Hz);

      // Failure
      auto fail0 = hal::calculate_can_bus_divider(500'000 * Hz, 100'000 * Hz);
      auto fail1 = hal::calculate_can_bus_divider(500'000 * Hz, 250'000 * Hz);
      auto fail2 = hal::calculate_can_bus_divider(500'000 * Hz, 500'000 * Hz);
      auto fail3 = hal::calculate_can_bus_divider(500'000 * Hz, 1'000'000 * Hz);
      auto fail4 =
        hal::calculate_can_bus_divider(8'000'000 * Hz, 1'250'000 * Hz);

      expect(that % not fail0.has_value());
      expect(that % not fail1.has_value());
      expect(that % not fail2.has_value());
      expect(that % not fail3.has_value());
      expect(that % not fail4.has_value());
    };
}

void message_finder_test()
{
  using namespace boost::ut;

  "hal::can_message_finder::find() two messages"_test = []() {
    // Setup
    constexpr hal::u32 search_id = 0x115;
    test_can_transceiver test_transceiver;
    hal::can_message_finder reader(test_transceiver, search_id);
    hal::can_message findable_message0{
      .id = search_id,
      .extended = false,
      .remote_request = false,
      .length = 3,
      .payload = { 0xAB, 0xCD, 0xEF },
    };

    hal::can_message findable_message1 = {
      .id = search_id,
      .extended = false,
      .remote_request = false,
      .length = 2,
      .payload = { 0xDE, 0xAD },
    };

    hal::can_message skipped_message = {
      .id = search_id + 5,
      .extended = false,
      .remote_request = false,
      .length = 1,
      .payload = { 0xCC },
    };

    // Ensure
    auto const ensure_message = reader.find();
    expect(not ensure_message.has_value());

    // Exercise
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(findable_message0);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(findable_message1);
    test_transceiver.add_to_received_messages(findable_message1);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(findable_message0);
    test_transceiver.add_to_received_messages(skipped_message);
    auto const found_message0 = reader.find();
    auto const found_message1 = reader.find();
    auto const found_message2 = reader.find();
    auto const found_message3 = reader.find();
    auto const no_message_found0 = reader.find();
    auto const no_message_found1 = reader.find();

    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(findable_message1);
    test_transceiver.add_to_received_messages(skipped_message);

    auto const found_message4 = reader.find();

    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);
    test_transceiver.add_to_received_messages(skipped_message);

    auto const no_message_found2 = reader.find();

    // Verify
    expect(found_message0.has_value());
    expect(found_message0.value() == findable_message0);
    expect(found_message1.has_value());
    expect(found_message1.value() == findable_message1);
    expect(found_message2.has_value());
    expect(found_message2.value() == findable_message1);
    expect(found_message3.has_value());
    expect(found_message3.value() == findable_message0);
    expect(found_message4.has_value());
    expect(found_message4.value() == findable_message1);
    expect(not no_message_found0.has_value());
    expect(not no_message_found1.has_value());
    expect(not no_message_found2.has_value());
  };

  "hal::can_message_finder::find() overflow cursor"_test = []() {
    // Setup
    constexpr hal::u32 expected_id = 0x115;
    test_can_transceiver test_transceiver;
    hal::can_message_finder reader(test_transceiver, expected_id);
    hal::can_message desired_message{
      .id = expected_id,
      .extended = false,
      .remote_request = false,
      .length = 3,
      .payload = { 0xAB, 0xCD, 0xEF },
    };

    hal::can_message undesired_message = desired_message;
    undesired_message.id = expected_id + 1;
    // Setup: With 12 messages to fill the buffer, the cursor will land back
    // at 0. Meaning that a find will come up with nothing. This is to be
    // expected with a circular buffer if the receive cursor gets lapped.

    // Ensure
    auto const ensure_message0 = reader.find();
    expect(not ensure_message0.has_value());
    auto const ensure_message1 = reader.find();
    expect(not ensure_message1.has_value());

    // Exercise
    test_transceiver.add_to_received_messages(desired_message);
    for (std::size_t i = 0; i < test_transceiver.m_buffer.size(); i++) {
      test_transceiver.add_to_received_messages(undesired_message);
    }

    auto const found_message0 = reader.find();

    // Verify
    expect(not found_message0.has_value());
  };

  "hal::can_message_finder::transceiver() should work"_test = []() {
    // Setup
    test_can_transceiver test_transceiver;
    hal::can_message_finder reader(test_transceiver, 0x111);
    hal::can_message expected_message{
      .id = 0x111,
      .extended = false,
      .remote_request = false,
      .length = 3,
      .payload = { 0xAB, 0xCD, 0xEF },
    };

    // Exercise
    auto future = reader.transceiver().send(ctx, expected_message);
    finish(future);

    // Verify
    expect(expected_message == test_transceiver.m_last_sent_message);
  };

  "hal::can_message_finder::id()"_test = []() {
    // Setup
    constexpr auto desired_id = 0x087;
    test_can_transceiver test_transceiver;
    hal::can_message_finder reader(test_transceiver, desired_id);

    // Exercise
    auto const actual_id = reader.id();

    // Verify
    expect(desired_id == actual_id);
  };
}

}  // namespace

int main()
{
  divider_test();
  message_finder_test();
}
