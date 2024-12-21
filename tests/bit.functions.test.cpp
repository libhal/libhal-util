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

boost::ut::suite<"bit_clear_test"> bit_clear_test = [] {
  using namespace boost::ut;

  "hal::bit clear() increment (0)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit clear() increment (1)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1;
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
  };

  "hal::bit clear() increment (2)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x2;
    expect(that % 0x0000'0002 ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.clear<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit clear() increment (10)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xA;
    expect(that % 0x0000'000A ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0008 ==
           control_register.clear<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0008 ==
           control_register.clear<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit clear() increment upper half (0x1'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1'FFFF;
    expect(that % 0x0000'FFFF ==
           control_register.clear<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0x0000'FFFF ==
           control_register.clear<bit_mask::from(17)>().to<std::uint32_t>());
    expect(that % 0x0000'FFFF ==
           control_register.clear<bit_mask::from(18)>().to<std::uint32_t>());
  };

  "hal::bit clear() increment lower half (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFF'FFFE ==
           control_register.clear<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFFC ==
           control_register.clear<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFF8 ==
           control_register.clear<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit clear() increment upper half (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFE'FFFF ==
           control_register.clear<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0xFFFC'FFFF ==
           control_register.clear<bit_mask::from(17)>().to<std::uint32_t>());
    expect(that % 0xFFF8'FFFF ==
           control_register.clear<bit_mask::from(18)>().to<std::uint32_t>());
  };

  "hal::bit clear() multiple bits (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFF'FF00 ==
           control_register.clear<bit_mask::from(0, 7)>().to<std::uint32_t>());
    expect(
      that % 0xFFE1'FF00 ==
      control_register.clear<bit_mask::from(17, 20)>().to<std::uint32_t>());
    expect(
      that % 0xE001'FF00 ==
      control_register.clear<bit_mask::from(21, 28)>().to<std::uint32_t>());
  };
};
boost::ut::suite<"bit_extract_test"> bit_extract_test = [] {
  using namespace boost::ut;

  "hal::bit extract() single input increment(0)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(0);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0x1 == extracted);
  };

  "hal::bit extract() single input increment(4)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(4);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0x0 == extracted);
  };

  "hal::bit extract() single input increment(8)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(8);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0x1 == extracted);
  };

  "hal::bit extract() double input increment(0)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(0, 1);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0x1 == extracted);
  };

  "hal::bit extract() double input increment(4)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(0, 3);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0xD == extracted);
  };

  "hal::bit extract() double input increment(8)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(0, 7);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0xCD == extracted);
  };

  "hal::bit extract() double input increment(4,7)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(4, 7);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0xC == extracted);
  };

  "hal::bit extract() double input increment(8,15)"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(8, 15);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0xAB == extracted);
  };

  "hal::bit extract() double input upper half"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(16, 23);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0x23 == extracted);
  };

  "hal::bit extract() double input out of range"_test = []() {
    std::uint32_t control_register = 0x0123'ABCD;
    constexpr auto extract_mask = bit_mask::from(24, 39);
    auto const extracted = bit_extract<extract_mask>(control_register);
    expect(that % 0x0001 == extracted);
  };
};

boost::ut::suite<"bit_insert_test"> bit_insert_test = [] {
  using namespace boost::ut;

  "hal::bit insert single input increment (0)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    static constexpr auto insert_mask = bit_mask::from(0);
    expect(that % 0x0000'0001 ==
           control_register.insert<insert_mask>(0xFFFFUL).to<std::uint32_t>());
  };

  "hal::bit insert single input increment (1)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    static constexpr auto insert_mask = bit_mask::from(1);
    expect(that % 0x0000'0002 ==
           control_register.insert<insert_mask>(0xFFFFUL).to<std::uint32_t>());
  };

  "hal::bit insert single input increment (16)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    static constexpr auto insert_mask = bit_mask::from(16);
    expect(that % 0x0001'0000 ==
           control_register.insert<insert_mask>(0xFFFFUL).to<std::uint32_t>());
  };

  "hal::bit insert double input increment (0)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit_mask::from(0, 15);
    expect(that % 0xFFFF'ABCD ==
           control_register.insert<insert_mask>(0xABCDUL).to<std::uint32_t>());
  };

  "hal::bit insert double input increment (1)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit_mask::from(1, 15);
    expect(that % 0xFFFF'579B ==
           control_register.insert<insert_mask>(0xABCDUL).to<std::uint32_t>());
  };

  "hal::bit insert double input increment (16)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit_mask::from(16, 31);
    expect(that % 0xABCD'FFFF ==
           control_register.insert<insert_mask>(0xABCDUL).to<std::uint32_t>());
  };

  "hal::bit insert double input increment out of range"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    static constexpr auto insert_mask = bit_mask::from(27, 42);
    expect(that % 0x6FFF'FFFF ==
           control_register.insert<insert_mask>(0xABCDUL).to<std::uint32_t>());
  };
};

boost::ut::suite<"bit_set_test"> bit_set_test = [] {
  using namespace boost::ut;

  "hal::bit set() increment (0)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    expect(that % 0x0000'0001 ==
           control_register.set<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0003 ==
           control_register.set<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0007 ==
           control_register.set<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit set() increment (1)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1;
    expect(that % 0x0000'0001 ==
           control_register.set<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0003 ==
           control_register.set<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0007 ==
           control_register.set<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit set() increment (2)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x2;
    expect(that % 0x0000'0003 ==
           control_register.set<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0003 ==
           control_register.set<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0007 ==
           control_register.set<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit set() increment (10)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xA;
    expect(that % 0x0000'000B ==
           control_register.set<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'000B ==
           control_register.set<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'000F ==
           control_register.set<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit set() increment upper half (0x1'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1'FFFF;
    expect(that % 0x0001'FFFF ==
           control_register.set<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0x0003'FFFF ==
           control_register.set<bit_mask::from(17)>().to<std::uint32_t>());
    expect(that % 0x0007'FFFF ==
           control_register.set<bit_mask::from(18)>().to<std::uint32_t>());
  };
};

boost::ut::suite<"bit_toggle_test"> bit_toggle_test = [] {
  using namespace boost::ut;

  "hal::bit toggle() increment (0)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    expect(that % 0x0000'0001 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0003 ==
           control_register.toggle<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0007 ==
           control_register.toggle<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate (0)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x0;
    expect(that % 0x0000'0001 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0001 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
  };

  "hal::bit toggle() increment (1)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1;
    expect(that % 0x0000'0000 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0002 ==
           control_register.toggle<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0006 ==
           control_register.toggle<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate (1)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1;
    expect(that % 0x0000'0000 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0001 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0000 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
  };

  "hal::bit toggle() increment (2)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x2;
    expect(that % 0x0000'0003 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0001 ==
           control_register.toggle<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'0005 ==
           control_register.toggle<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate (2)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x2;
    expect(that % 0x0000'0003 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0002 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0003 ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
  };

  "hal::bit toggle() increment (10)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xA;
    expect(that % 0x0000'000B ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'0009 ==
           control_register.toggle<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0x0000'000D ==
           control_register.toggle<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate (10)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xA;
    expect(that % 0x0000'000B ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'000A ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0x0000'000B ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
  };

  "hal::bit toggle() increment upper half (0x1'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1'FFFF;
    expect(that % 0x0000'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0x0002'FFFF ==
           control_register.toggle<bit_mask::from(17)>().to<std::uint32_t>());
    expect(that % 0x0006'FFFF ==
           control_register.toggle<bit_mask::from(18)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate upper half (0x1'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0x1'FFFF;
    expect(that % 0x0000'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0x0001'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0x0000'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
  };

  "hal::bit toggle() increment lower half (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFF'FFFE ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFFC ==
           control_register.toggle<bit_mask::from(1)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFF8 ==
           control_register.toggle<bit_mask::from(2)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate lower half (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFF'FFFE ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFFF ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFFE ==
           control_register.toggle<bit_mask::from(0)>().to<std::uint32_t>());
  };

  "hal::bit toggle() increment upper half (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFE'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0xFFFC'FFFF ==
           control_register.toggle<bit_mask::from(17)>().to<std::uint32_t>());
    expect(that % 0xFFF8'FFFF ==
           control_register.toggle<bit_mask::from(18)>().to<std::uint32_t>());
  };

  "hal::bit toggle() alternate upper half (0xFFFF'FFFF)"_test = []() {
    hal::bit_value<std::uint32_t> control_register = 0xFFFF'FFFF;
    expect(that % 0xFFFE'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0xFFFF'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
    expect(that % 0xFFFE'FFFF ==
           control_register.toggle<bit_mask::from(16)>().to<std::uint32_t>());
  };
};

boost::ut::suite<"bit_mask_shift_test"> bit_mask_shift_test = [] {
  using namespace boost::ut;

  "hal::bit_mask shift right"_test = []() {
    constexpr auto mask1 = hal::bit_mask::from(0, 7);
    constexpr auto mask2 = mask1 << 8U;
    constexpr auto mask3 = mask2 << 8U;
    constexpr auto mask4 = mask3 << 8U;

    expect(that % 0 == mask1.position);
    expect(that % 8 == mask1.width);

    expect(that % 8 == mask2.position);
    expect(that % 8 == mask2.width);

    expect(that % 16 == mask3.position);
    expect(that % 8 == mask3.width);

    expect(that % 24 == mask4.position);
    expect(that % 8 == mask4.width);
  };

  "hal::bit_mask left right"_test = []() {
    constexpr auto mask1 = hal::bit_mask::from(23, 26);
    constexpr auto mask2 = mask1 >> 5U;
    constexpr auto mask3 = mask2 >> 5U;
    constexpr auto mask4 = mask3 >> 5U;

    expect(that % 23 == mask1.position);
    expect(that % 4 == mask1.width);

    expect(that % (23 - 5) == mask2.position);
    expect(that % 4 == mask2.width);

    expect(that % (23 - (5 * 2)) == mask3.position);
    expect(that % 4 == mask3.width);

    expect(that % (23 - (5 * 3)) == mask4.position);
    expect(that % 4 == mask4.width);
  };
};
}  // namespace hal
