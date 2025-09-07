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
#include <print>
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
}  // namespace hal::v5::usb
