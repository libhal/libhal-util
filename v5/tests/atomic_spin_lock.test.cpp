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

#include <libhal-util/atomic_spin_lock.hpp>

#include <stdexcept>
#include <thread>

#include <libhal-util/mock/steady_clock.hpp>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite atomic_spin_lock_test = []() {
  using namespace boost::ut;
  using namespace std::chrono_literals;

  "hal::atomic_spin_lock"_test = []() {
    "::lock & ::unlock"_test = []() {
      // Setup
      hal::atomic_spin_lock test_subject;
      // Setup: Take the lock pre-before the thread can take it.
      test_subject.lock();
      bool thread_started = false;
      bool thread_ended = false;

      std::thread lock_taker_thread([&] {
        thread_started = true;
        test_subject.lock();
        thread_ended = true;
      });

      while (not thread_started) {
        std::this_thread::sleep_for(1ms);
      }

      std::this_thread::sleep_for(1ms);

      // Exercise
      expect(that % not thread_ended);
      test_subject.unlock();
      std::this_thread::sleep_for(1ms);
      lock_taker_thread.join();

      // Verify
      expect(that % thread_ended);
    };

    "::try_lock"_test = []() {
      // Setup
      hal::atomic_spin_lock test_subject;
      test_subject.lock();

      // Exercise + Verify
      expect(not test_subject.try_lock());
      expect(not test_subject.try_lock());
      expect(not test_subject.try_lock());
      expect(not test_subject.try_lock());

      test_subject.unlock();

      expect(test_subject.try_lock());
      expect(not test_subject.try_lock());
    };
  };

  "hal::timed_atomic_spin_lock"_test = []() {
    "::lock"_test = []() {
      // Setup
      using namespace std::chrono_literals;
      mock_steady_clock mock_steady_clock;
      hal::timed_atomic_spin_lock test_subject(mock_steady_clock);

      // Setup: Take the lock pre-before the thread can take it.
      test_subject.lock();
      bool thread_started = false;
      bool thread_ended = false;

      std::thread lock_taker_thread([&] {
        thread_started = true;
        test_subject.lock();
        thread_ended = true;
      });

      while (not thread_started) {
        std::this_thread::sleep_for(1ms);
      }

      std::this_thread::sleep_for(1ms);

      // Exercise
      expect(that % not thread_ended);
      test_subject.unlock();
      std::this_thread::sleep_for(1ms);
      lock_taker_thread.join();

      // Verify
      expect(that % thread_ended);
    };

    "::try_lock"_test = []() {
      // Setup
      mock_steady_clock mock_steady_clock;
      hal::timed_atomic_spin_lock test_subject(mock_steady_clock);
      test_subject.lock();

      // Exercise + Verify
      expect(not test_subject.try_lock());
      expect(not test_subject.try_lock());
      expect(not test_subject.try_lock());
      expect(not test_subject.try_lock());

      test_subject.unlock();

      expect(test_subject.try_lock());
      expect(not test_subject.try_lock());
    };
    "::try_lock_for"_test = []() {
      // Setup
      mock_steady_clock mock_steady_clock;
      // Setup: Use 1kHz to make each uptime tick 1ms.
      mock_steady_clock.set_frequency(1.0_kHz);
      std::queue<std::uint64_t> uptime_queue;
      // Setup: Add enough uptime ticks to fork for the time below.
      for (int i = 0; i < 4; i++) {
        uptime_queue.push(i);
      }
      mock_steady_clock.set_uptimes(uptime_queue);
      hal::timed_atomic_spin_lock test_subject(mock_steady_clock);
      bool dead_locked = true;
      std::thread dead_lock_checker([&dead_locked] {
        std::this_thread::sleep_for(10ms);
        if (dead_locked) {
          throw std::runtime_error("Test dead locked!");
        }
      });

      // Exercise
      test_subject.lock();
      auto lock_acquired_0 = test_subject.try_lock_for(2ms);
      test_subject.unlock();
      auto lock_acquired_1 = test_subject.try_lock_for(2ms);
      dead_locked = false;
      dead_lock_checker.join();

      // Verify
      expect(that % not lock_acquired_0);
      expect(that % lock_acquired_1);
    };
  };
};
}  // namespace hal
