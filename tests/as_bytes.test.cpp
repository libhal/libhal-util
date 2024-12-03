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

#include <libhal-util/as_bytes.hpp>

#include <span>
#include <vector>

#include <boost/ut.hpp>

namespace hal {
boost::ut::suite<"as_bytes_test"> as_bytes_test = [] {
  using namespace boost::ut;

  "hal::as_bytes()"_test = []() {
    "Zero"_test = []() {
      // Setup
      std::span<hal::byte const> expected_const{};
      std::span<hal::byte> expected_writable{};
      std::vector<float> empty_vector;

      {
        // Exercise
        auto actual = hal::as_bytes(std::span<int>{});
        // Verify
        expect(that % expected_const.data() == actual.data());
        expect(that % expected_const.size() == actual.size());
      }
      {
        // Exercise
        auto actual = hal::as_bytes(empty_vector);
        // Verify
        expect(that % expected_const.data() == actual.data());
        expect(that % expected_const.size() == actual.size());
      }
      {
        // Exercise
        auto actual = hal::as_writable_bytes(expected_writable);
        // Verify
        expect(that % expected_writable.data() == actual.data());
        expect(that % expected_writable.size() == actual.size());
      }
    };

    "One"_test = []() {
      // Setup
      std::array<std::int32_t, 1> array{ 1234 };
      std::vector<std::int32_t> vector;
      vector.push_back(1);

      auto* array_pointer = reinterpret_cast<hal::byte const*>(array.data());
      auto* vector_pointer = reinterpret_cast<hal::byte const*>(vector.data());

      auto vector_byte_size =
        sizeof(decltype(vector)::value_type) * vector.size();
      auto array_byte_size = sizeof(decltype(array)::value_type) * array.size();

      {
        // Exercise
        auto actual = hal::as_bytes(array);
        // Verify
        expect(that % array_pointer == actual.data());
        expect(that % array_byte_size == actual.size());
      }
      {
        // Exercise
        auto actual = hal::as_bytes(vector);
        // Verify
        expect(that % vector_pointer == actual.data());
        expect(that % vector_byte_size == actual.size());
      }
      {
        // Exercise
        auto actual = hal::as_writable_bytes(array);
        // Verify
        expect(that % array_pointer == actual.data());
        expect(that % array_byte_size == actual.size());
      }
    };

    "Standard Usage"_test = []() {
      // Setup
      std::array<std::int32_t, 17> array{ 1234 };
      std::vector<std::int32_t> vector;
      vector.push_back(1);
      vector.push_back(2);
      vector.push_back(3);
      vector.push_back(4);
      vector.push_back(10293);

      auto* array_pointer = reinterpret_cast<hal::byte const*>(array.data());
      auto* vector_pointer = reinterpret_cast<hal::byte const*>(vector.data());

      constexpr auto array_element_size = sizeof(decltype(array)::value_type);
      constexpr auto array_byte_size = array_element_size * array.size();

      constexpr auto vector_element_size = sizeof(decltype(vector)::value_type);
      auto const vector_byte_size = vector_element_size * vector.size();

      {
        // Exercise
        auto actual = hal::as_bytes(array);
        // Verify
        expect(that % array_pointer == actual.data());
        expect(that % array_byte_size == actual.size());
      }
      {
        // Exercise
        auto actual = hal::as_bytes(vector);
        // Verify
        expect(that % vector_pointer == actual.data());
        expect(that % vector_byte_size == actual.size());
      }
      {
        // Exercise
        auto actual = hal::as_writable_bytes(array);
        // Verify
        expect(that % array_pointer == actual.data());
        expect(that % array_byte_size == actual.size());
      }
    };
  };
};
}  // namespace hal
