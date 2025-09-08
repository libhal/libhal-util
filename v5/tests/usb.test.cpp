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

#include "libhal-util/usb/descriptors.hpp"
#include "libhal-util/usb/utils.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <libhal-util/usb.hpp>

#include <boost/ut.hpp>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>
#include <memory_resource>
#include <span>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace hal::v5::usb {
namespace {

// constexpr setup_packet set_addr_master{ false,
//                                         setup_packet::type::standard,
//                                         setup_packet::recipient::device,
//                                         static_cast<byte>(
//                                           standard_request_types::set_address),
//                                         0x30,
//                                         0,  // a
//                                         0 };

constexpr u8 iface_desc_length = 9;
constexpr u8 iface_desc_type = 0x4;
constexpr u8 str_desc_type = 0x3;
constexpr u8 iad_length = 0x08;
constexpr u8 iad_type = 0x0B;

template<typename T>
size_t scatter_span_size(scatter_span<T> ss)
{
  size_t res = 0;
  for (auto const& s : ss) {
    res += s.size();
  }

  return res;
}

template<typename T>
bool span_eq(std::span<T> const lhs, std::span<T> const rhs)
{
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.size(); i++) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template<typename T>
bool span_ne(std::span<T> const lhs, std::span<T> const rhs)
{
  return !(lhs == rhs);
}

constexpr std::vector<hal::byte> pkt_to_scatter(setup_packet const& req)
{
  std::vector<byte> vec;
  vec.push_back(req.request_type);
  vec.push_back(req.request);
  vec.append_range(setup_packet::to_le_bytes(req.value));
  vec.append_range(setup_packet::to_le_bytes(req.index));
  vec.append_range(setup_packet::to_le_bytes(req.length));

  return vec;
}

class mock_usb_endpoint : public usb::endpoint
{
public:
  usb::endpoint_info m_info{};
  bool m_stall_called{ false };
  bool m_should_stall{ false };
  bool m_reset_called{ false };

protected:
  [[nodiscard]] usb::endpoint_info driver_info() const override
  {
    return m_info;
  }

  void driver_stall(bool p_should_stall) override
  {
    m_stall_called = true;
    m_should_stall = p_should_stall;
  }

  void driver_reset() override
  {
    m_reset_called = true;
  }
};

class mock_usb_control_endpoint : public control_endpoint
{
public:
  mock_usb_endpoint m_endpoint;
  bool m_is_connected{ false };
  u8 m_address{ 0 };
  std::vector<byte> m_out_buf;
  std::array<byte, 8> m_req_buf;
  usize m_read_result{ 0 };
  callback<void(on_receive_tag)> m_receive_callback{};

  void write_request(scatter_span<byte const> p_data)
  {
    size_t total_len = 0;
    for (std::span<byte const> const& s : p_data) {
      for (byte const el : s) {
        m_req_buf[total_len] = el;
        total_len++;
      }
    }
  }

  void simulate_interrupt()
  {
    m_receive_callback(on_receive_tag{});
  }

private:
  [[nodiscard]] endpoint_info driver_info() const override
  {
    return m_endpoint.info();
  }

  void driver_stall(bool p_should_stall) override
  {
    m_endpoint.stall(p_should_stall);
  }

  void driver_connect(bool p_should_connect) override
  {
    m_is_connected = p_should_connect;
  }

  void driver_set_address(u8 p_address) override
  {
    m_address = p_address;
  }

  void driver_write(scatter_span<byte const> p_data) override
  {
    // Ignore ZLPs
    if (p_data.empty() ||
        (scatter_span_size(p_data) == 1 && p_data[0][0] == 0)) {
      return;
    }
    for (std::span<byte const> const& s : p_data) {
      for (byte const el : s) {
        m_out_buf.push_back(el);
      }
    }
  }

  usize driver_read(scatter_span<byte> p_buffer) override
  {
    size_t src_idx = 0;
    for (auto s : p_buffer) {
      for (unsigned char& el : s) {
        el = m_req_buf[src_idx];
        src_idx++;
      }
    }

    return src_idx;
  }

  void driver_on_receive(
    callback<void(on_receive_tag)> const& p_callback) override
  {
    m_receive_callback = p_callback;
  }

  void driver_reset() override
  {
    m_endpoint.reset();
  }
};

struct mock : public interface
{

  constexpr mock(std::string_view p_name)
    : name(p_name)
  {
  }

  [[nodiscard]] descriptor_count driver_write_descriptors(
    descriptor_start p_start,
    endpoint_writer const& p_callback) override
  {
    byte res_iface_idx = 0;
    if (p_start.interface.has_value()) {
      interface_number() = p_start.interface.value();
      res_iface_idx = 1;
    }

    byte res_str_idx = 0;
    if (p_start.string.has_value()) {
      interface_name_string_idx() = p_start.string.value();
      res_str_idx = 1;
    }

    p_callback(make_scatter_bytes(m_packed_array));

    return { .interface = res_iface_idx, .string = res_str_idx };
  }

  [[nodiscard]] bool driver_write_string_descriptor(
    u8 p_index,
    endpoint_writer const& p_callback) override
  {
    if (p_index != interface_name_string_idx()) {
      return false;
    }
    std::array<byte const, 2> str_hdr(
      { static_cast<byte const>(descriptor_type::string),
        static_cast<byte const>(name.length()) });

    auto scatter_arr = make_scatter_bytes(
      str_hdr,
      std::span(reinterpret_cast<byte const*>(name.data()), name.length()));

    p_callback(scatter_span<byte const>(scatter_arr));

    return true;
  }

  bool driver_handle_request(setup_packet const& p_setup,
                             endpoint_writer const& p_callback) override
  {
    std::ignore = p_setup;
    std::ignore = p_callback;
    return true;
  }

  constexpr byte& interface_number()
  {
    return m_packed_array[2];
  }

  constexpr byte& alt_settings()
  {
    return m_packed_array[3];
  }

  constexpr byte& interface_name_string_idx()
  {
    return m_packed_array[8];
  }

  std::array<u8, 9> m_packed_array = {
    iface_desc_length,
    iface_desc_type,
    0,  // interface_number
    0,  // alternate_setting
    1,  //  num_endpoints
    2,  //  interface_class
    3,  // interface_sub_class
    4,  // interface_protocol
    0   // interface name index
  };
  std::string_view name;
};

class iad_mock : public interface
{
public:
  ~iad_mock() override = default;

  iad_mock(std::string_view p_iface_name_one,  // NOLINT
           std::string_view p_iface_name_two)
    : m_name_one(p_iface_name_one)
    , m_name_two(p_iface_name_two) {};

  struct mock_iface_descriptor
  {
    byte num;
    byte alt_settings;
    byte str_idx;
    bool feature;
  };

private:
  [[nodiscard]] descriptor_count driver_write_descriptors(
    descriptor_start p_start,
    endpoint_writer const& p_callback) override
  {
    if (!p_start.interface.has_value() || !p_start.string.has_value()) {
      throw hal::argument_out_of_domain(this);
    }

    auto iface_idx = p_start.interface.value();
    auto str_idx = p_start.string.value();
    std::array<byte const, 8> iad_buf{
      iad_length, iad_type,
      0,         // first interface
      2,         // iface count
      0,         // class
      0,         // subclass
      0,         // proto
      str_idx++  // string idx
    };

    std::array<byte const, 2> iface_header = { iface_desc_length,
                                               iface_desc_type };
    std::array<byte const, 5> static_desc_vars = {
      0,  // altsettings
      1,  // num endpoints
      0,  // class
      0,  // subclass
      0,  // protocol
    };

    m_iface_one = { .num = iface_idx++,
                    .alt_settings = 0,
                    .str_idx = str_idx++,
                    .feature = false };
    m_iface_two = { .num = iface_idx++,
                    .alt_settings = 0,
                    .str_idx = str_idx++,
                    .feature = false };

    std::array<byte const, 1> iface_one_arr({ m_iface_one.num });
    std::array<byte const, 1> iface_one_str_arr({ m_iface_one.str_idx });

    std::array<byte const, 1> iface_two_arr({ m_iface_two.num });
    std::array<byte const, 1> iface_two_str_arr({ m_iface_two.str_idx });

    auto span_arr = make_scatter_bytes(iad_buf,

                                       // iface one
                                       iface_header,
                                       std::span(iface_one_arr),
                                       static_desc_vars,
                                       std::span(iface_one_str_arr),

                                       // iface two
                                       iface_header,
                                       std::span(iface_two_arr),
                                       static_desc_vars,
                                       std::span(iface_two_str_arr));

    p_callback(span_arr);
    m_wrote_descriptors = true;
    return { .interface = 2, .string = 3 };
  }

  [[nodiscard]] bool driver_write_string_descriptor(
    u8 p_index,
    endpoint_writer const& p_callback) override
  {
    if (!m_wrote_descriptors) {
      safe_throw(hal::operation_not_permitted(this));
    }
    std::array<byte, 2> header{ 0, str_desc_type };
    if (p_index == m_iface_one.str_idx) {
      header[0] = m_name_one.length() + 2;
      auto arr = make_scatter_bytes(
        header,
        std::span(reinterpret_cast<byte const*>(m_name_one.data()),
                  m_name_one.length()));
      p_callback(arr);
      return true;
    } else if (p_index == m_iface_two.str_idx) {
      header[0] = m_iface_two.str_idx + 2;
      auto arr = make_scatter_bytes(
        header,
        std::span(reinterpret_cast<byte const*>(m_name_two.data()),
                  m_name_two.length()));

      p_callback(arr);
      return true;
    }

    return false;
  }

  u16 driver_get_status(setup_packet p_pkt)
  {
    if (p_pkt.get_recipient() != setup_packet::recipient::interface) {
      // std::println("Unsupported recipient");
      safe_throw(hal::operation_not_supported(this));
    }

    auto iface_idx = p_pkt.index & 0xFF;

    if (iface_idx == m_iface_one.num) {
      return m_iface_one.num;
    } else if (iface_idx == m_iface_two.num) {
      return m_iface_two.num;
    }

    safe_throw(hal::operation_not_supported(this));
  }

  void manage_features(setup_packet p_pkt, bool p_set, u16 p_selector)
  {
    std::ignore = p_selector;
    if (p_pkt.get_recipient() != setup_packet::recipient::interface) {
      // std::println("Unsupported recipient");
      safe_throw(hal::operation_not_supported(this));
    }

    auto iface_idx = p_pkt.index & 0xFF;

    if (iface_idx == m_iface_one.num) {
      m_iface_one.feature = p_set;
    } else if (iface_idx == m_iface_two.num) {
      m_iface_two.feature = p_set;
    } else {
      // std::println("Invalid interface index");
      safe_throw(hal::operation_not_supported(this));
    }
  }

  u8 driver_get_interface(setup_packet p_pkt)
  {
    auto iface_idx = p_pkt.index & 0xFF;

    if (iface_idx == m_iface_one.num) {
      return m_iface_one.alt_settings;
    } else if (iface_idx == m_iface_two.num) {
      return m_iface_two.alt_settings;
    } else {
      // std::println("Invalid interface index");
      safe_throw(hal::operation_not_supported(this));
    }
  }

  void driver_set_interface(setup_packet p_pkt)
  {
    auto iface_idx = p_pkt.index & 0xFF;
    auto alt_setting = p_pkt.value;
    if (iface_idx == m_iface_one.num) {
      m_iface_one.alt_settings = alt_setting;
    } else if (iface_idx == m_iface_two.num) {
      m_iface_two.alt_settings = alt_setting;
    } else {
      // std::println("Invalid interface index");
      safe_throw(hal::operation_not_supported(this));
    }
  }

  bool driver_handle_request(setup_packet const& p_setup,
                             endpoint_writer const& p_callback) override
  {
    std::ignore = p_setup;
    std::ignore = p_callback;
    return false;
  }

public:
  mock_iface_descriptor m_iface_one;
  std::string_view m_name_one;
  mock_iface_descriptor m_iface_two;
  std::string_view m_name_two;
  bool m_wrote_descriptors = false;
};

void simulate_sending_payload(
  strong_ptr<mock_usb_control_endpoint> const& ctrl_ptr,
  setup_packet& req)
{
  auto vec = pkt_to_scatter(req);
  auto scatter_arr = make_scatter_bytes(vec);
  scatter_span<byte const> ss(scatter_arr);

  ctrl_ptr->write_request(ss);
}

// u8 calculate_conf_desc_recursive(std::span<configuration> p_conf_arr)
// {
//   u8 total_len = 0;
//   for (configuration& conf : p_conf_arr) {
//     total_len += 9;
//     for (auto& iface : conf.interfaces) {
//       auto real_iface = dynamic_cast<mock*>(&(*iface));
//       std::ignore = iface->write_descriptors(
//         { .interface = 0, .string = real_iface->interface_name_string_idx()
//         },
//         [&total_len](scatter_span<byte const> p_data) {
//           total_len += scatter_span_size(p_data);
//         });
//     }
//   }

//   return total_len;
// }

// std::vector<byte> generate_conf_descriptors(std::span<configuration>
// p_conf_arr)
// {
//   std::vector<byte> vec;
//   for (configuration const& conf : p_conf_arr) {
//     vec.append_range(std::span<byte const>(conf));
//     for (auto const& iface : conf.interfaces) {
//       std::ignore =
//         iface->write_descriptors({ .interface = 0, .string = 0 },
//                                  [&vec](scatter_span<byte const> p_dat) {
//                                    for (auto const& s : p_dat) {
//                                      for (auto const& el : s) {
//                                        vec.push_back(el);
//                                      }
//                                    }
//                                  });
//     }
//   }

//   return vec;
// }

}  // namespace
boost::ut::suite<"usb_test"> usb_test = [] {
  // TODO(#78): Add usb utility tests
};

boost::ut::suite<"enumeration_test"> enumeration_test = [] {
  using namespace boost::ut;
  using namespace hal::literals;
  namespace pmr = std::pmr;
  static std::array<byte, 4096> iface_buf;

  iface_buf.fill(0);
  device dev({ .p_bcd_usb = 0x0002,
               .p_device_class = class_code::application_specific,
               .p_device_subclass = 0,
               .p_device_protocol = 0,
               .p_id_vendor = 0xffff,
               .p_id_product = 0x0000,
               .p_bcd_device = 0x0001,
               .p_manufacturer = "libhal",
               .p_product = "Unit Test",
               .p_serial_number_str = "123456789" });
  pmr::monotonic_buffer_resource pool(iface_buf.data(), std::size(iface_buf));
  std::array<configuration, 1> conf_arr{ configuration{
    "Test Config",
    configuration::bitmap(true, false),
    1,
    &pool,
    make_strong_ptr<mock>(&pool, mock("Mock Iface")) } };

  mock_usb_control_endpoint ctrl_ep;
  ctrl_ep.m_endpoint.m_info = { .size = 8, .number = 0, .stalled = false };
  auto ctrl_ptr = make_strong_ptr<mock_usb_control_endpoint>(&pool, ctrl_ep);
  enumerator<1> en{
    ctrl_ptr, device(dev), std::array(conf_arr), "LANG", 1,
    false  // NOLINT
  };

  "basic usb enumeration test"_test = [&en, &ctrl_ptr] {
    // Start enumeration process and verify connection
    auto f = [&en]() { en.enumerate(); };
    constexpr byte delay_time_ms = 1000;
    auto& ctrl_buf = ctrl_ptr->m_out_buf;
    std::thread ejh(f);
    std::this_thread::sleep_for(std::chrono::milliseconds(
      delay_time_ms));  // Should be enough time to connect
    expect(that % true == ctrl_ptr->m_is_connected);
    ctrl_buf.clear();

    u16 expected_addr = 0x30;
    setup_packet set_addr{ false,
                           setup_packet::type::standard,
                           setup_packet::recipient::device,
                           static_cast<byte>(
                             standard_request_types::set_address),
                           0x30,
                           0,  // a
                           0 };

    simulate_sending_payload(ctrl_ptr, set_addr);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    expect(that % expected_addr == ctrl_ptr->m_address);
    ctrl_buf.clear();

    // Get device descriptor
    u16 desc_t_idx = static_cast<byte>(descriptor_type::device) << 8;
    setup_packet get_desc(
      true,
      setup_packet::type::standard,
      setup_packet::recipient::device,
      static_cast<byte>(standard_request_types::get_descriptor),
      desc_t_idx,
      0,
      18);
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

    expect(that % (span_eq(std::span<byte const>(dev_expected), dev_actual)));
    ctrl_buf.clear();

    // Get a string descriptor header from device
    // where 1 is the manufacture string index
    constexpr u16 str_desc_t_idx =
      static_cast<byte>(descriptor_type::string) << 8 | 1;
    setup_packet str_hdr_req(
      true,
      setup_packet::type::standard,
      setup_packet::recipient::device,
      static_cast<byte>(standard_request_types::get_descriptor),
      str_desc_t_idx,
      0,
      2);

    simulate_sending_payload(ctrl_ptr, str_hdr_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    std::array<byte const, 2> expected_str_hdr{
      static_cast<byte const>(6),  // string is "libhal"
      static_cast<byte const>(descriptor_type::string)
    };
    std::span<byte const> actual_dev_str_hdr(ctrl_buf.data(), 2);
    expect(that % (span_eq(std::span<byte const>(expected_str_hdr),
                           actual_dev_str_hdr)));
    ctrl_buf.clear();

    // Get a string descriptor from device
    setup_packet str_req(str_hdr_req);
    str_req.length = expected_str_hdr[0];
    simulate_sending_payload(ctrl_ptr, str_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    std::string_view expected_manu_str = "libhal";
    std::array<byte const, 2> manu_str_hdr(
      { static_cast<byte const>(expected_manu_str.length()),
        static_cast<byte const>(descriptor_type::string) });
    auto expected_manu_str_scatter =
      make_scatter_bytes(manu_str_hdr,
                         std::span<byte const>(reinterpret_cast<byte const*>(
                                                 expected_manu_str.data()),
                                               expected_manu_str.length()));
    auto actual_manu_str_scatter = make_scatter_bytes(ctrl_buf);
    expect(that % (scatter_span<byte const>(expected_manu_str_scatter) ==
                   scatter_span<byte const>(actual_manu_str_scatter)));
    ctrl_buf.clear();

    // Get Configuration length
    setup_packet conf_hdr_req(
      true,
      setup_packet::type::standard,
      setup_packet::recipient::device,
      static_cast<byte>(standard_request_types::get_descriptor),
      static_cast<byte>(descriptor_type::configuration) << 8,
      0,
      2);

    // Expected Config + interface descriptor
    std::array<byte const, 18> expected_conf_iface_desc{
      // config descriptor
      0x9,                                                      // len
      static_cast<byte const>(descriptor_type::configuration),  // type
      0x12,  // TODO: total length
      0x0,   // tl
      0x1,   // number of interfaces
      0x0,   // config value
      0x4,   // name string index
      0xc0,  // bmattributes (selfpowered = true)
      0x1,   // max power

      // Interface descriptor
      iface_desc_length,
      iface_desc_type,
      0x0,  // iface number
      0x0,  // alt settings
      0x1,  // number of endpoints
      0x2,  // class
      0x3,  // subclass
      0x4,  // protocol
      0x5   // iface name string index
    };

    simulate_sending_payload(ctrl_ptr, conf_hdr_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));

    auto expected_tl_hh = expected_conf_iface_desc[2];
    auto expected_tl_ll = expected_conf_iface_desc[3];
    auto expected_total_len =
      setup_packet::from_le_bytes(expected_tl_hh, expected_tl_ll);

    auto actual_tl_hh = ctrl_buf[0];
    auto actual_tl_ll = ctrl_buf[1];
    auto actual_total_len =
      setup_packet::from_le_bytes(actual_tl_hh, actual_tl_ll);
    expect(that % expected_total_len == actual_total_len);
    ctrl_buf.clear();

    // Get Configuration Descriptor + interface descriptor + endpoint descriptor
    setup_packet conf_req(conf_hdr_req);
    conf_req.length = expected_total_len;
    simulate_sending_payload(ctrl_ptr, conf_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));
    expect(that % (span_eq(std::span<byte const>(expected_conf_iface_desc),
                           std::span<byte const>(ctrl_buf))));

    // Set configuration
    setup_packet set_conf_req(
      false,
      setup_packet::type::standard,
      setup_packet::recipient::device,
      static_cast<byte const>(standard_request_types::set_configuration),
      0,
      0,
      0);

    simulate_sending_payload(ctrl_ptr, set_conf_req);
    ctrl_ptr->simulate_interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_time_ms));

    ejh.join();
    // Verify active config
    expect(that %
           (span_eq(std::span(expected_conf_iface_desc.data() + 2, 7),
                    std::span<byte const>(en.get_active_configuration()))));
  };
};

}  // namespace hal::v5::usb
