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

#include <libhal-util/bit.hpp>

#include <boost/ut.hpp>

namespace hal {

void bit_clear_test()
{
  using namespace boost::ut;

  "hal::bit clear() increment (0)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<1>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<2>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
  };

  "hal::bit clear() increment (1)"_test = []() {
    volatile std::uint32_t control_register = 0x1;
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<1>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<2>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
  };

  "hal::bit clear() increment (2)"_test = []() {
    volatile std::uint32_t control_register = 0x2;
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0x0000'0002 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<1>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<2>();
    expect(that % 0x0000'0000 == control_register.clear<clear_mask>());
  };

  "hal::bit clear() increment (10)"_test = []() {
    volatile std::uint32_t control_register = 0xA;
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0x0000'000A == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0x0000'0008 == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0x0000'0008 == control_register.clear<clear_mask>());
  };

  "hal::bit clear() increment upper half (0x1'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0x1'FFFF;
    constexpr auto clear_mask = bit::mask::from<16>();
    expect(that % 0x0000'FFFF == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<17>();
    expect(that % 0x0000'FFFF == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<18>();
    expect(that % 0x0000'FFFF == control_register.clear<clear_mask>());
  };

  "hal::bit clear() increment lower half (0xFFFF'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    constexpr auto clear_mask = bit::mask::from<0>();
    expect(that % 0xFFFF'FFFE == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<1>();
    expect(that % 0xFFFF'FFFC == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<2>();
    expect(that % 0xFFFF'FFF8 == control_register.clear<clear_mask>());
  };

  "hal::bit clear() increment upper half (0xFFFF'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    constexpr auto clear_mask = bit::mask::from<16>();
    expect(that % 0xFFFE'FFFF == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<17>();
    expect(that % 0xFFFC'FFFF == control_register.clear<clear_mask>());
    constexpr auto clear_mask = bit::mask::from<18>();
    expect(that % 0xFFF8'FFFF == control_register.clear<clear_mask>());
  };
};

void bit_extract_test()
{
  using namespace boost::ut;

  "hal::bit extract() single input increment(0)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<0>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0x1 == extracted);
  };

  "hal::bit extract() single input increment(4)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<4>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0x0 == extracted);
  };

  "hal::bit extract() single input increment(8)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<8>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0x1 == extracted);
  };

  "hal::bit extract() double input increment(0)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<0, 1>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0x1 == extracted);
  };

  "hal::bit extract() double input increment(4)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<0, 3>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0xD == extracted);
  };

  "hal::bit extract() double input increment(8)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<0, 7>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0xCD == extracted);
  };

  "hal::bit extract() double input increment(4,7)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<4, 7>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0xC == extracted);
  };

  "hal::bit extract() double input increment(8,15)"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<8, 15>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0xAB == extracted);
  };

  "hal::bit extract() double input upper half"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<16, 23>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0x23 == extracted);
  };

  "hal::bit extract() double input out of range"_test = []() {
    volatile std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit::mask::from<24, 39>();
    auto extracted = bit::extract<extract_mask>(control_register);
    expect(that % 0x0001 == extracted);
  };
};

void bit_insert_test()
{
  using namespace boost::ut;

  "hal::bit insert single input increment (0)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    static constexpr auto insert_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 ==
           control_register.insert<insert_mask>(0xFFFFUL));
  };

  "hal::bit insert single input increment (1)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    static constexpr auto insert_mask = bit::mask::from<1>();
    expect(that % 0x0000'0002 ==
           control_register.insert<insert_mask>(0xFFFFUL));
  };

  "hal::bit insert single input increment (16)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    static constexpr auto insert_mask = bit::mask::from<16>();
    expect(that % 0x0001'0000 ==
           control_register.insert<insert_mask>(0xFFFFUL));
  };

  "hal::bit insert double input increment (0)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit::mask::from<0, 15>();
    expect(that % 0xFFFF'ABCD ==
           control_register.insert<insert_mask>(0xABCDUL));
  };

  "hal::bit insert double input increment (1)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit::mask::from<1, 15>();
    expect(that % 0xFFFF'579B ==
           control_register.insert<insert_mask>(0xABCDUL));
  };

  "hal::bit insert double input increment (16)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit::mask::from<16, 31>();
    expect(that % 0xABCD'FFFF ==
           control_register.insert<insert_mask>(0xABCDUL));
  };

  "hal::bit insert double input increment out of range"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit::mask::from<27, 42>();
    expect(that % 0x6FFF'FFFF ==
           control_register.insert<insert_mask>(0xABCDUL));
  };
};

void bit_set_test()
{
  using namespace boost::ut;

  "hal::bit set() increment (0)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    constexpr auto set_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<1>();
    expect(that % 0x0000'0003 == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<2>();
    expect(that % 0x0000'0007 == control_register.set<set_mask>());
  };

  "hal::bit set() increment (1)"_test = []() {
    volatile std::uint32_t control_register = 0x1;
    constexpr auto set_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<1>();
    expect(that % 0x0000'0003 == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<2>();
    expect(that % 0x0000'0007 == control_register.set<set_mask>());
  };

  "hal::bit set() increment (2)"_test = []() {
    volatile std::uint32_t control_register = 0x2;
    constexpr auto set_mask = bit::mask::from<0>();
    expect(that % 0x0000'0003 == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<1>();
    expect(that % 0x0000'0003 == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<2>();
    expect(that % 0x0000'0007 == control_register.set<set_mask>());
  };

  "hal::bit set() increment (10)"_test = []() {
    volatile std::uint32_t control_register = 0xA;
    constexpr auto set_mask = bit::mask::from<0>();
    expect(that % 0x0000'000B == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<1>();
    expect(that % 0x0000'000B == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<2>();
    expect(that % 0x0000'000F == control_register.set<set_mask>());
  };

  "hal::bit set() increment upper half (0x1'FFFF)"_test = []() {
    xstd::bitset test_set(0x1'FFFF);
    volatile std::uint32_t control_register = 0x1'FFFF;
    constexpr auto set_mask = bit::mask::from<16>();
    expect(that % 0x0001'FFFF == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<17>();
    expect(that % 0x0003'FFFF == control_register.set<set_mask>());
    constexpr auto set_mask = bit::mask::from<18>();
    expect(that % 0x0007'FFFF == control_register.set<set_mask>());
  };
};

void bit_toggle_test()
{
  using namespace boost::ut;

  "hal::bit toggle() increment (0)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<1>();
    expect(that % 0x0000'0003 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<2>();
    expect(that % 0x0000'0007 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate (0)"_test = []() {
    volatile std::uint32_t control_register = 0x0;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0000 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() increment (1)"_test = []() {
    volatile std::uint32_t control_register = 0x1;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0000 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<1>();
    expect(that % 0x0000'0002 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<2>();
    expect(that % 0x0000'0006 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate (1)"_test = []() {
    volatile std::uint32_t control_register = 0x1;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0000 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0001 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0000 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() increment (2)"_test = []() {
    volatile std::uint32_t control_register = 0x2;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0003 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<1>();
    expect(that % 0x0000'0001 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<2>();
    expect(that % 0x0000'0005 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate (2)"_test = []() {
    volatile std::uint32_t control_register = 0x2;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0003 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0002 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'0003 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() increment (10)"_test = []() {
    volatile std::uint32_t control_register = 0xA;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'000B == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<1>();
    expect(that % 0x0000'0009 == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<2>();
    expect(that % 0x0000'000D == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate (10)"_test = []() {
    volatile std::uint32_t control_register = 0xA;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'000B == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'000A == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0x0000'000B == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() increment upper half (0x1'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0x1'FFFF;
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0x0000'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<17>();
    expect(that % 0x0002'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<18>();
    expect(that % 0x0006'FFFF == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate upper half (0x1'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0x1'FFFF;
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0x0000'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0x0001'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0x0000'FFFF == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() increment lower half (0xFFFF'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0xFFFF'FFFE == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<1>();
    expect(that % 0xFFFF'FFFC == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<2>();
    expect(that % 0xFFFF'FFF8 == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate lower half (0xFFFF'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0xFFFF'FFFE == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0xFFFF'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<0>();
    expect(that % 0xFFFF'FFFE == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() increment upper half (0xFFFF'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0xFFFE'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<17>();
    expect(that % 0xFFFC'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<18>();
    expect(that % 0xFFF8'FFFF == control_register.toggle<toggle_mask>());
  };

  "hal::bit toggle() alternate upper half (0xFFFF'FFFF)"_test = []() {
    volatile std::uint32_t control_register = 0xFFFF'FFFF;
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0xFFFE'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0xFFFF'FFFF == control_register.toggle<toggle_mask>());
    constexpr auto toggle_mask = bit::mask::from<16>();
    expect(that % 0xFFFE'FFFF == control_register.toggle<toggle_mask>());
  };
};
}  // namespace hal