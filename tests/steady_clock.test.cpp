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

#include <libhal-util/steady_clock.hpp>

#include <boost/ut.hpp>

// #include <libhal/testing.hpp>

namespace hal {
boost::ut::suite<"steady_clock"> steady_clock_test = [] {
  using namespace boost::ut;

  // Make the frequency equal to inverse of the time duration period, giving you
  // the frequency of each tick of time_duration.
  static constexpr auto expected_frequency =
    static_cast<hertz>(hal::time_duration::period::den);

  class dummy_steady_clock : public hal::steady_clock
  {
  public:
    std::uint64_t m_uptime = 0;

  private:
    hertz driver_frequency() override
    {
      return expected_frequency;
    }

    std::uint64_t driver_uptime() override
    {
      return m_uptime++;
    }
  };

  // =============== timeout ===============

  "hal::create_timeout(hal::steady_clock, 0ns)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(0);
    dummy_steady_clock test_steady_clock;

    // Exercise
    auto timeout_object = create_timeout(test_steady_clock, expected);

    expect(throws<hal::timed_out>([&timeout_object]() { timeout_object(); }))
      << "throws hal::timed_out";

    // Verify
    // Verify: subtract 2 because 2 invocations are required in order to get
    //         the start uptime and another to check what the latest uptime is.
    expect(that % expected.count() == (test_steady_clock.m_uptime - 2));
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::create_timeout(hal::steady_clock, 50ns)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(50);
    dummy_steady_clock test_steady_clock;

    // Exercise
    auto timeout_object = create_timeout(test_steady_clock, expected);

    expect(throws<hal::timed_out>([&timeout_object]() {
      // Terminate the loop one iteration before the timeout would occur
      for (std::int64_t i = 0; i < expected.count(); i++) {
        timeout_object();
      }
    }))
      << "hal::timed_out::timed_out was not thrown!";

    // Verify
    // After the last call to uptime() the uptime value is incremented by one
    expect(that % expected.count() == test_steady_clock.m_uptime - 1);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::create_timeout(hal::steady_clock, 1337ns)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(10);
    dummy_steady_clock test_steady_clock;

    // Exercise
    auto timeout_object = create_timeout(test_steady_clock, expected);

    expect(throws<hal::timed_out>([&timeout_object]() {
      for (std::int64_t i = 0; i < expected.count(); i++) {
        timeout_object();
      }
    }))
      << "hal::timed_out::timed_out was not thrown!";

    // Verify
    expect(that % expected.count() == test_steady_clock.m_uptime - 1);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::create_timeout(hal::steady_clock, -5ns) returns error"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(-5);
    dummy_steady_clock test_steady_clock;

    // Exercise
    [[maybe_unused]] auto result = create_timeout(test_steady_clock, expected);

    // Verify
    expect(that % 1 == test_steady_clock.m_uptime);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  // =============== delay ===============

  "hal::delay(hal::steady_clock, 0ns)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(0);
    dummy_steady_clock test_steady_clock;

    // Exercise
    delay(test_steady_clock, expected);

    // Verify
    // Verify: subtract 2 because 2 invocations are required in order to get
    //         the start uptime and another to check what the latest uptime
    //         is.
    expect(that % expected.count() == (test_steady_clock.m_uptime - 2));
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::delay(hal::steady_clock, 50ns)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(50);
    dummy_steady_clock test_steady_clock;

    // Exercise
    delay(test_steady_clock, expected);

    // Verify
    expect(that % expected.count() == test_steady_clock.m_uptime - 1);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::delay(hal::steady_clock, 1337ns)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(1'337);
    dummy_steady_clock test_steady_clock;

    // Exercise
    delay(test_steady_clock, expected);

    // Verify
    expect(that % expected.count() == test_steady_clock.m_uptime - 1);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::delay(hal::steady_clock, -5ns) returns error"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(-5);
    dummy_steady_clock test_steady_clock;

    // Exercise
    delay(test_steady_clock, expected);

    // Verify
    // Verify: Adjust uptime by 2 because at least 2 calls to uptime()
    expect(that % 0 == test_steady_clock.m_uptime - 2);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::timeout_generator(hal::steady_clock)"_test = []() {
    // Setup
    static constexpr hal::time_duration expected(50);
    dummy_steady_clock test_steady_clock;

    // Exercise
    auto generator = timeout_generator(test_steady_clock);
    auto timeout_object = generator(expected);

    expect(throws<hal::timed_out>([&timeout_object]() {
      // Terminate the loop one iteration before the timeout would occur
      for (std::int64_t i = 0; i < expected.count(); i++) {
        timeout_object();
      }
    }))
      << "hal::timed_out::timed_out was not thrown!";

    // Verify
    // After the last call to uptime() the uptime value is incremented by one
    expect(that % expected.count() == test_steady_clock.m_uptime - 1);
    expect(that % expected_frequency == test_steady_clock.frequency());
  };

  "hal::future_deadline(hal::steady_clock, relative_time)"_test = []() {
    // Setup
    using namespace std::chrono_literals;

    dummy_steady_clock test_steady_clock;
    static constexpr auto expected = 1'000'000;  // 1ms in nanoseconds
    test_steady_clock.m_uptime = 0;

    // Exercise
    auto future_deadline_value = hal::future_deadline(test_steady_clock, 1ms);

    // Verify
    expect(that % expected == future_deadline_value);
  };
};
}  // namespace hal
