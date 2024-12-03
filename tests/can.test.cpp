#include <libhal-util/can.hpp>

#include <iostream>

#include <boost/ut.hpp>

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

  auto test_subject = call_result.value();

  auto const bit_width = hal::bit_width(test_subject);

  auto calculated_baud_rate =
    p_operating_frequency /
    (test_subject.clock_divider *
     (test_subject.sync_segment + test_subject.propagation_delay +
      test_subject.phase_segment1 + test_subject.phase_segment2));

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
}  // namespace

namespace hal {
boost::ut::suite<"can"> can_test = [] {
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
}
}  // namespace hal
