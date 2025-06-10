#include <libhal-util/can.hpp>

#include <iostream>

#include <boost/ut.hpp>
#include <libhal/can.hpp>
#include <ratio>

namespace hal {

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
  std::basic_ostream<CharT, Traits>& p_ostream,
  hal::can_bus_divider_t const& p_settings)
{
  p_ostream << "{ clock_divider: " << int(p_settings.clock_divider);
  p_ostream << ", sync: " << int(p_settings.sync_segment);
  p_ostream << ", propagation_delay: " << int(p_settings.propagation_delay);
  p_ostream << ", phase_segment1: " << int(p_settings.phase_segment1);
  p_ostream << ", phase_segment2: " << int(p_settings.phase_segment2);
  p_ostream << ", sjw: " << int(p_settings.synchronization_jump_width);
  p_ostream << ", total_tq: " << int(p_settings.total_tq);
  p_ostream << " }";
  return p_ostream;
}

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(
  std::basic_ostream<CharT, Traits>& p_ostream,
  std::optional<hal::can_bus_divider_t> const& p_settings)
{
  if (p_settings) {
    p_ostream << p_settings.value();
  } else {
    p_ostream << "std::nullopt { hal::can_bus_divider_t }";
  }
  return p_ostream;
}
}  // namespace hal

namespace {
void check_validity(hal::hertz p_operating_frequency,
                    hal::hertz p_target_baud_rate)
{
  using namespace boost::ut;
  auto call_result =
    hal::calculate_can_bus_divider(p_operating_frequency, p_target_baud_rate);

  expect(that % call_result.has_value())
    << "Frequency '" << p_operating_frequency << "' and baud rate '"
    << p_target_baud_rate << "'\n";

  // clang-tidy for some reason doesn't accept that `.value()` as "checked"
  // usage of an optional.
  auto test_subject = call_result.value();  // NOLINT

  auto const bit_width = hal::bit_width(test_subject);

  auto const calculated_bit_width =
    (test_subject.sync_segment + test_subject.propagation_delay +
     test_subject.phase_segment1 + test_subject.phase_segment2);
  auto const denominator =
    static_cast<float>(test_subject.clock_divider * calculated_bit_width);
  auto const calculated_baud_rate = p_operating_frequency / denominator;

  expect(that % 8 <= bit_width && bit_width <= 25)
    << "Bit width is beyond the bounds of 8 and 25";
  expect(that % 8 <= test_subject.total_tq && test_subject.total_tq <= 25)
    << "Total tq is beyond the bounds of 8 and 25";
  expect(that % bit_width == test_subject.total_tq)
    << "bit_width and total_tq do not match";
  expect(that % static_cast<std::uint32_t>(calculated_baud_rate) ==
         static_cast<std::uint32_t>(p_target_baud_rate))
    << "Failure to get the expected baud rate with " << test_subject;
}

struct test_can_transceiver : hal::can_transceiver
{
  test_can_transceiver() = default;
  hal::u32 driver_baud_rate() override
  {
    using namespace hal::literals;
    return 100_kHz;
  }

  void driver_send(hal::can_message const& p_message) override
  {
    last_sent_message = p_message;
  }

  std::span<hal::can_message const> driver_receive_buffer() override
  {
    return m_buffer;
  }

  std::size_t driver_receive_cursor() override
  {
    return m_cursor % m_buffer.size();
  };

  void add_to_received_messages(hal::can_message const& p_message)
  {
    m_buffer[m_cursor++ % m_buffer.size()] = p_message;
  }

  hal::can_message last_sent_message{};
  std::array<hal::can_message, 12> m_buffer{};
  std::size_t m_cursor = 0;
};
}  // namespace

namespace hal {
boost::ut::suite<"can_test"> can_test = [] {
  using namespace boost::ut;

  "operator==(can::message, can::message) "_test = []() {
    can::message_t a = {
      .id = 0x111,
      .payload = { 0xAA },
      .length = 1,
      .is_remote_request = false,
    };

    can::message_t b = {
      .id = 0x111,
      .payload = { 0xAA },
      .length = 1,
      .is_remote_request = false,
    };

    can::message_t c = {
      .id = 0x112,
      .payload = { 0xAB },
      .length = 1,
      .is_remote_request = false,
    };

    expect(a == b);
    expect(a != c);
    expect(b != c);
  };

  "calculate_can_bus_divider(operation_frequency, desired_baud_rate) "_test =
    []() {
      check_validity(8.0_MHz, 100.0_kHz);
      check_validity(8.0_MHz, 250.0_kHz);
      check_validity(8.0_MHz, 500.0_kHz);
      check_validity(8.0_MHz, 1000.0_kHz);

      check_validity(16.0_MHz, 100.0_kHz);
      check_validity(16.0_MHz, 250.0_kHz);
      check_validity(16.0_MHz, 500.0_kHz);
      check_validity(16.0_MHz, 1000.0_kHz);

      check_validity(16.0_MHz, 100.0_kHz);
      check_validity(16.0_MHz, 250.0_kHz);
      check_validity(16.0_MHz, 500.0_kHz);
      check_validity(16.0_MHz, 1000.0_kHz);

      check_validity(46.0_MHz, 100.0_kHz);
      check_validity(46.0_MHz, 250.0_kHz);
      check_validity(46.0_MHz, 500.0_kHz);
      check_validity(46.0_MHz, 1000.0_kHz);

      check_validity(64.0_MHz, 100.0_kHz);
      check_validity(64.0_MHz, 250.0_kHz);
      check_validity(64.0_MHz, 500.0_kHz);
      check_validity(64.0_MHz, 1000.0_kHz);

      check_validity(96.0_MHz, 100.0_kHz);
      check_validity(96.0_MHz, 250.0_kHz);
      check_validity(96.0_MHz, 500.0_kHz);
      check_validity(96.0_MHz, 1000.0_kHz);

      check_validity(120.0_MHz, 100.0_kHz);
      check_validity(120.0_MHz, 250.0_kHz);
      check_validity(120.0_MHz, 500.0_kHz);
      check_validity(120.0_MHz, 1000.0_kHz);

      // Failure
      auto fail0 = calculate_can_bus_divider(500.0_kHz, 100.0_kHz);
      auto fail1 = calculate_can_bus_divider(500.0_kHz, 250.0_kHz);
      auto fail2 = calculate_can_bus_divider(500.0_kHz, 500.0_kHz);
      auto fail3 = calculate_can_bus_divider(500.0_kHz, 1000.0_kHz);
      auto fail4 = calculate_can_bus_divider(8.0_MHz, 1250.0_kHz);

      std::cout << "fail0" << fail0 << "\n";
      std::cout << "fail1" << fail1 << "\n";
      std::cout << "fail2" << fail2 << "\n";
      std::cout << "fail3" << fail3 << "\n";
      std::cout << "fail4" << fail4 << "\n";

      expect(that % not fail0.has_value());
      expect(that % not fail1.has_value());
      expect(that % not fail2.has_value());
      expect(that % not fail4.has_value());
    };

  "hal::can_message_finder"_test = []() {
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
      // Setup: With 8 messages to fill the buffer, the cursor will land back at
      // 0. Meaning that a find will come up with nothing. This is to be
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

    "hal::can_message_finder::find() std::nullopt from stability failure"_test =
      []() {
        // This is empty because we haven't come up with a practical cross
        // platform way to verify this check.
      };

    "hal::can_message_finder::transceiver() should work"_test = []() {
      // Setup
      test_can_transceiver test_transceiver;
      hal::can_message_finder reader(test_transceiver, 0x111);
      constexpr hal::can_message expected_message{
        .id = 0x111,
        .extended = false,
        .remote_request = false,
        .length = 3,
        .payload = { 0xAB, 0xCD, 0xEF },
      };
      // Setup: With 8 messages to fill the buffer, the cursor will land back at
      // 0. Meaning that a find will come up with nothing. This is to be
      // expected with a circular buffer if the receive cursor gets lapped.

      // Exercise
      reader.transceiver().send(expected_message);

      // Verify
      expect(expected_message == test_transceiver.last_sent_message);
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
  };
};
}  // namespace hal
