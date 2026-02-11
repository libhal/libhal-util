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

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <memory_resource>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

#include <libhal-util/mock/usb.hpp>
#include <libhal-util/usb.hpp>
#include <libhal-util/usb/descriptors.hpp>
#include <libhal-util/usb/utils.hpp>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

#include <boost/ut.hpp>

namespace hal::v5::usb {

boost::ut::suite<"usb_test"> usb_test = [] {
  // TODO(#78): Add usb utility tests
};

boost::ut::suite<"enumeration_test"> enumeration_test = [] {
  using namespace boost::ut;
  using namespace hal::literals;
  namespace pmr = std::pmr;
  static std::array<byte, 4096> iface_buf;

  iface_buf.fill(0);
  device dev({ .bcd_usb = 0x0002,
               .device_class = class_code::application_specific,
               .device_subclass = 0,
               .device_protocol = 0,
               .id_vendor = 0xffff,
               .id_product = 0x0000,
               .bcd_device = 0x0001,
               .p_manufacturer = u"libhal",
               .p_product = u"Unit Test",
               .p_serial_number_str = u"123456789" });

  pmr::monotonic_buffer_resource pool(iface_buf.data(), std::size(iface_buf));
  std::array<configuration, 1> conf_arr{ configuration{
    { .name = u"Test Config",
      .attributes = configuration::bitmap(true, false),
      .max_power = 1,
      .allocator = &pool },
    make_strong_ptr<mock>(&pool, mock(u"Mock Iface")) } };

  mock_usb_control_endpoint ctrl_ep;
  ctrl_ep.m_endpoint.m_info = { .size = 8, .number = 0, .stalled = false };
  auto ctrl_ptr = make_strong_ptr<mock_usb_control_endpoint>(&pool, ctrl_ep);
  auto const conf_arr_ptr =
    make_strong_ptr<std::array<configuration, 1>>(&pool, conf_arr);
  auto dev_ptr = make_strong_ptr<device>(&pool, dev);
  auto console_ptr = make_strong_ptr<mock_serial>(&pool, mock_serial());

  "basic usb enumeration test"_test = [&ctrl_ptr, &dev_ptr, &conf_arr_ptr] {
    // Start enumeration process and verify connection
    std::optional<enumerator<1>> en;
    auto f = [&en, &ctrl_ptr, &dev_ptr, &conf_arr_ptr]() {
      en.emplace(enumerator<1>::args{ .ctrl_ep = ctrl_ptr,
                                      .device = dev_ptr,
                                      .configs = conf_arr_ptr,
                                      .lang_str = 0x0409 });
      en->enumerate();
    };
    constexpr byte delay_time_ms = 1000;
    auto& ctrl_buf = ctrl_ptr->m_out_buf;
    std::thread ejh(f);
    std::this_thread::sleep_for(std::chrono::milliseconds(
      delay_time_ms));  // Should be enough time to connect
    expect(that % true == ctrl_ptr->m_is_connected);
    ctrl_buf.clear();

    u16 expected_addr = 0x30;
    setup_packet set_addr{
      { .device_to_host = false,
        .type = setup_packet::request_type::standard,
        .recipient = setup_packet::request_recipient::device,
        .request = static_cast<byte>(standard_request_types::set_address),
        .value = 0x30,
        .index = 0,  // a
        .length = 0 }
    };

    simulate_sending_payload(ctrl_ptr, set_addr);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    expect(that % expected_addr == ctrl_ptr->m_address);
    ctrl_buf.clear();

    // Get device descriptor
    u16 desc_t_idx = static_cast<byte>(descriptor_type::device) << 8;
    setup_packet get_desc(
      { .device_to_host = true,
        .type = setup_packet::request_type::standard,
        .recipient = setup_packet::request_recipient::device,
        .request = static_cast<byte>(standard_request_types::get_descriptor),
        .value = desc_t_idx,
        .index = 0,
        .length = 18 });
    simulate_sending_payload(ctrl_ptr, get_desc);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    std::span<byte const> dev_actual(ctrl_buf.data(), 18);
    std::array<byte, 18> dev_expected{
      0x12,                                              // length
      static_cast<byte const>(descriptor_type::device),  // type
      0x02,                                              // usb bcd
      0x00,
      static_cast<byte const>(class_code::application_specific),
      0,     // subclass
      0,     // protocol
      8,     // max packet size
      0xff,  // vendor id
      0xff,
      0,  // product id
      0,
      0x1,  // bcd device
      0x0,
      1,  // manufactures index
      2,  // product index
      3,  // product index
      1   // num configuration
    };

    expect(std::ranges::equal(std::span<byte const>(dev_expected), dev_actual));
    ctrl_buf.clear();

    // Get a string descriptor header from device
    // where 1 is the manufacture string index
    constexpr u16 str_desc_t_idx =
      static_cast<byte>(descriptor_type::string) << 8 | 1;
    setup_packet str_hdr_req(
      { .device_to_host = true,
        .type = setup_packet::request_type::standard,
        .recipient = setup_packet::request_recipient::device,
        .request = static_cast<byte>(standard_request_types::get_descriptor),
        .value = str_desc_t_idx,
        .index = 0,
        .length = 2 });

    simulate_sending_payload(ctrl_ptr, str_hdr_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    std::array<byte const, 2> expected_manu_str_hdr{
      static_cast<byte const>(14),  // string is "libhal"
      static_cast<byte const>(descriptor_type::string)
    };
    std::span<byte const> actual_dev_str_hdr(ctrl_buf.data(), 2);
    expect(std::ranges::equal(std::span<byte const>(expected_manu_str_hdr),
                              actual_dev_str_hdr));
    ctrl_buf.clear();

    // Get a string descriptor from device
    setup_packet str_req(str_hdr_req);
    str_req.length(expected_manu_str_hdr[0]);
    simulate_sending_payload(ctrl_ptr, str_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    std::u16string_view expected_manu_str = u"libhal";

    auto expected_manu_str_scatter =
      make_scatter_bytes(expected_manu_str_hdr,
                         std::span<byte const>(reinterpret_cast<byte const*>(
                                                 expected_manu_str.data()),
                                               expected_manu_str.length() * 2));
    auto actual_manu_str_scatter = make_scatter_bytes(ctrl_buf);
    expect(that % (scatter_span<byte const>(expected_manu_str_scatter) ==
                   scatter_span<byte const>(actual_manu_str_scatter)));
    ctrl_buf.clear();

    // Get Configuration length
    setup_packet conf_hdr_req(
      { .device_to_host = true,
        .type = setup_packet::request_type::standard,
        .recipient = setup_packet::request_recipient::device,
        .request = static_cast<byte>(standard_request_types::get_descriptor),
        .value = static_cast<byte>(descriptor_type::configuration) << 8,
        .index = 0,
        .length = 9 });

    // Expected Config + interface descriptor
    std::array<byte const, 18> expected_conf_iface_desc{
      // config descriptor
      0x9,                                                      // len
      static_cast<byte const>(descriptor_type::configuration),  // type
      0x12,  // HH: total length
      0x0,   // LL: tl
      0x1,   // number of interfaces
      0x1,   // config value
      0x4,   // name string index
      0xc0,  // bmattributes (selfpowered = true)
      0x1,   // max power

      // Interface descriptor
      interface_description_length,
      interface_description_type,
      0x0,  // iface number
      0x0,  // alt settings
      0x1,  // number of endpoints
      0x2,  // class
      0x3,  // subclass
      0x4,  // protocol
      0x5   // iface name string index
    };

    // Get Configuration descriptor header
    simulate_sending_payload(ctrl_ptr, conf_hdr_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));

    auto expected_tl_hh = expected_conf_iface_desc[2];
    auto expected_tl_ll = expected_conf_iface_desc[3];
    auto expected_total_len =
      setup_packet::from_le_bytes(expected_tl_hh, expected_tl_ll);

    auto actual_tl_hh = ctrl_buf[2];
    auto actual_tl_ll = ctrl_buf[3];
    auto actual_total_len =
      setup_packet::from_le_bytes(actual_tl_hh, actual_tl_ll);
    expect(that % expected_total_len == actual_total_len);
    ctrl_buf.clear();

    // Get Configuration Descriptor + interface descriptor + endpoint descriptor
    setup_packet conf_req(conf_hdr_req);
    conf_req.length(expected_total_len);
    simulate_sending_payload(ctrl_ptr, conf_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    expect(std::ranges::equal(std::span<byte const>(expected_conf_iface_desc),
                              std::span<byte const>(ctrl_buf)));

    // Set configuration
    setup_packet set_conf_req(
      { .device_to_host = false,
        .type = setup_packet::request_type::standard,
        .recipient = setup_packet::request_recipient::device,
        .request =
          static_cast<byte const>(standard_request_types::set_configuration),
        .value = 1,
        .index = 0,
        .length = 0 });

    simulate_sending_payload(ctrl_ptr, set_conf_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));

    ejh.join();
    // Verify active config
    expect(std::ranges::equal(
      std::span(expected_conf_iface_desc.data() + 2, 7),
      std::span<byte const>(en->get_active_configuration())));
  };
};

}  // namespace hal::v5::usb
