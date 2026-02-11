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

#pragma once

#include <cstddef>
#include <cstdlib>
#include <cwchar>

#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

#include "descriptors.hpp"
#include "libhal-util/as_bytes.hpp"
#include "libhal-util/usb/endpoints.hpp"
#include "utils.hpp"

namespace hal::v5::usb {

template<typename T>
constexpr size_t scatter_span_size(scatter_span<T> ss)
{
  size_t res = 0;
  for (auto const& s : ss) {
    res += s.size();
  }

  return res;
}

/**
 * @brief Result of make_sub_scatter_array containing the scatter span array
 * and the number of valid spans within it.
 */
template<typename T, size_t N>
struct sub_scatter_result
{
  // Array of spans composing the sub scatter span
  std::array<std::span<T>, N> spans;
  // Number of valid spans in the array (may be less than N if truncated)
  size_t count;
};

// TODO: Create proper scatter span data structure and remove this
/**
 * @brief Create a sub scatter span from span fragments. Meaning only create a
 * composite scatter span of a desired length instead of being composed of every
 * span.
 *
 * eg:
 * first_span = {1, 2, 3}
 * second_span = {4, 5, 6, 7}
 * new_sub_span = make_scatter_span_array(5, first_span, second_span) => {1, 2,
 * 3, 4, 5}
 *
 * Unfortunately, it is not as clean as the above psuedo code. In reality
 * this function returns the required spans of a given sub scatter span and the
 * number of required spans. As of this time starting location is always the
 * start of the first span given
 *
 * Usage:
 * @code{.cpp}
 * std::array<byte, 3> first = {1, 2, 3};
 * std::array<byte, 4> second = {4, 5, 6, 7};
 * std::array<byte, 2> third = {8, 9};
 *
 * // Request only 5 bytes from 9 total
 * auto result = make_sub_scatter_bytes(5, first, second, third);
 *
 * // result.spans -> Spans in sub scatter span
 * // result.count -> Number of scatter spans needed to account for desired
 * elements
 *
 * // Use count to limit the scatter_span to valid spans only
 * auto ss = scatter_span<byte const>(result.spans).first(result.count);
 * @endcode
 *
 * @tparam T - The type each span contains
 * @tparam Args... - The spans to compose the scatter span
 *
 * @param p_count - Number of elements (of type T) desired for scatter span
 * @param p_spans - The spans that will be used to compose the new sub scatter
 * span.
 *
 * @return A sub_scatter_result containing the spans used within the
 * scatter_span and the number of spans in the sub scatter span.
 */
template<typename T, typename... Args>
constexpr sub_scatter_result<T, sizeof...(Args)> make_sub_scatter_array(
  size_t p_count,
  Args&&... p_spans)
{
  std::array<std::span<T>, sizeof...(Args)> full_ss{ std::span<T>(
    std::forward<Args>(p_spans))... };

  size_t total_span_len = scatter_span_size(scatter_span<T>(full_ss));
  std::array<std::span<T>, sizeof...(Args)> res;
  std::array<size_t, sizeof...(Args)> lens{ std::span<T>(p_spans).size()... };

  if (total_span_len <= p_count) {
    return { .spans = full_ss, .count = full_ss.size() };
  }
  size_t cur_len = 0;
  size_t i = 0;
  for (; i < lens.size(); i++) {
    auto ith_span_length = lens[i];

    if (p_count >= (cur_len + ith_span_length)) {
      res[i] = full_ss[i];
      cur_len += ith_span_length;
      continue;
    }

    if (cur_len >= p_count) {
      return { .spans = res, .count = i };
    }

    auto delta = p_count - cur_len;
    std::span<T> subspan = std::span(full_ss[i]).first(delta);
    res[i] = subspan;
    break;
  }

  return { .spans = res, .count = i + 1 };
}

/**
 * @brief Convenience wrapper for make_sub_scatter_array with byte const type.
 * @see make_sub_scatter_array
 */
template<typename... Args>
constexpr sub_scatter_result<byte const, sizeof...(Args)>
make_sub_scatter_bytes(size_t p_count, Args&&... p_spans)
{
  return make_sub_scatter_array<byte const>(p_count,
                                            std::forward<Args>(p_spans)...);
}

template<size_t num_configs>
class enumerator
{

public:
  struct args
  {
    strong_ptr<control_endpoint> ctrl_ep;
    strong_ptr<device> device;
    strong_ptr<std::array<configuration, num_configs>> configs;
    u16 lang_str;
  };

  enumerator(args p_args)
    : m_ctrl_ep(p_args.ctrl_ep)
    , m_device(p_args.device)
    , m_configs(p_args.configs)
    , m_lang_str(p_args.lang_str)
  {
  }

  void enumerate()
  {
    // Renumerate, a config will only be set if
    if (m_active_conf != nullptr) {
      m_active_conf = nullptr;
      m_ctrl_ep->connect(false);
    }

    // String indexes 1-3 are reserved for device descriptor strings
    // (manufacturer, product, serial number). Configuration strings start at 4.
    u8 cur_str_idx = 4;
    byte cur_iface_idx = 0;
    // Phase one: Preperation

    // Device
    m_device->set_num_configurations(num_configs);

    // Configurations
    for (size_t i = 0; i < num_configs; i++) {
      configuration& config = m_configs->at(i);
      config.set_configuration_index(cur_str_idx++);
      config.set_configuration_value(i + 1);
    }

    for (configuration& config : *m_configs) {
      auto total_length =
        static_cast<u16>(constants::configuration_descriptor_size);
      for (auto const& iface : config.interfaces) {
        auto deltas = iface->write_descriptors(
          { .interface = cur_iface_idx, .string = cur_str_idx },
          [&total_length](scatter_span<hal::byte const> p_data) {
            total_length += scatter_span_size(p_data);
          });

        cur_iface_idx += deltas.interface;
        cur_str_idx += deltas.string;
      }
      config.set_num_interfaces(cur_iface_idx);
      config.set_total_length(total_length);
    }

    // Phase two: Writing

    // TODO: Make async
    bool finished_enumeration = false;
    bool volatile waiting_for_data = true;

    using on_receive_tag = control_endpoint::on_receive_tag;

    m_ctrl_ep->on_receive(
      [&waiting_for_data](on_receive_tag) { waiting_for_data = false; });
    m_ctrl_ep->connect(true);

    // std::array<hal::byte, constants::size_std_req> raw_req;
    setup_packet req;
    do {
      // Seriously, make this async
      while (waiting_for_data) {
        continue;
      }
      waiting_for_data = true;

      auto scatter_raw_req = make_writable_scatter_bytes(req.raw_request_bytes);
      auto num_bytes_read = m_ctrl_ep->read(scatter_raw_req);

      if (num_bytes_read == 0) {
        continue;
      }

      if (num_bytes_read != constants::standard_request_size) {

        safe_throw(hal::message_size(num_bytes_read, this));
      }
      //
      // for (auto const& el : raw_req) {
      //
      // }
      //

      if (req.get_recipient() != setup_packet::request_recipient::device) {
        safe_throw(hal::not_connected(this));
      }

      // TODO: Handle exception
      handle_standard_device_request(req);
      // m_ctrl_ep->write({});  // Send ZLP to complete Data
      // Transaction
      if (static_cast<standard_request_types>(req.request()) ==
          standard_request_types::set_configuration) {
        finished_enumeration = true;
        m_ctrl_ep->on_receive(
          [this](on_receive_tag) { m_has_setup_packet = true; });
      }
    } while (!finished_enumeration);
  }

  [[nodiscard]] configuration& get_active_configuration()
  {
    if (m_active_conf == nullptr) {
      safe_throw(hal::operation_not_permitted(this));
    }

    return *m_active_conf;
  }

  void resume_ctrl_transaction()
  {
    while (!m_has_setup_packet) {
      continue;
    }

    m_has_setup_packet = false;

    setup_packet req;
    auto& read_buf = req.raw_request_bytes;
    auto scatter_read_buf = make_writable_scatter_bytes(read_buf);
    auto bytes_read = m_ctrl_ep->read(scatter_read_buf);
    std::span payload(read_buf.data(), bytes_read);

    if (req.get_type() == setup_packet::request_type::invalid) {
      return;
    }

    if (determine_standard_request(req) ==
          standard_request_types::get_descriptor &&
        static_cast<descriptor_type>(req.value() >> 8 & 0xFF) ==
          descriptor_type::string) {
      handle_str_descriptors(req.value() & 0xFF, req.length() > 2);

    } else if (req.get_recipient() == setup_packet::request_recipient::device) {
      handle_standard_device_request(req);
    } else {
      // Handle iface level requests
      interface::endpoint_writer f;
      if (req.is_device_to_host()) {
        f = [this](scatter_span<hal::byte const> resp) {
          m_ctrl_ep->write(resp);
        };
      } else {
        f = [this](scatter_span<hal::byte> resp) {
          std::ignore = m_ctrl_ep->read(
            resp);  // Can't use this... TODO: Maybe add a return for callbacks
                    // for "bytes processed"
        };
      }
      bool req_handled = false;
      for (auto const& iface : get_active_configuration()) {
        req_handled = iface->handle_request(req, f);
        if (req_handled) {
          break;
        }
      }
      m_ctrl_ep->write(
        {});  // A ZLP to terminate Data Transaction just to be safe

      if (!req_handled) {
        safe_throw(hal::argument_out_of_domain(this));
      }
    }
  }

private:
  void handle_standard_device_request(setup_packet& req)
  {
    switch (determine_standard_request(req)) {
      case standard_request_types::set_address: {
        m_ctrl_ep->write({});
        m_ctrl_ep->set_address(req.value());

        break;
      }

      case standard_request_types::get_descriptor: {
        process_get_descriptor(req);
        break;
      }

      case standard_request_types::get_configuration: {
        if (m_active_conf == nullptr) {
          safe_throw(hal::operation_not_permitted(this));
        }
        auto conf_value = m_active_conf->configuration_value();
        auto scatter_conf = make_scatter_bytes(std::span(&conf_value, 1));
        m_ctrl_ep->write(scatter_conf);
        break;
      }

      case standard_request_types::set_configuration: {
        m_active_conf = &(m_configs->at(req.value() - 1));
        break;
      }

      case standard_request_types::invalid:
      default:
        safe_throw(hal::not_connected(this));
    }
  }

  void process_get_descriptor(setup_packet& req)
  {
    hal::byte desc_type = req.value() >> 8 & 0xFF;
    [[maybe_unused]] hal::byte desc_idx = req.value() & 0xFF;

    switch (static_cast<descriptor_type>(desc_type)) {
      case descriptor_type::device: {
        auto header =
          std::to_array({ constants::device_descriptor_size,
                          static_cast<byte>(descriptor_type::device) });
        m_device->set_max_packet_size(
          static_cast<byte>(m_ctrl_ep->info().size));
        auto scatter_arr_pair =
          make_sub_scatter_bytes(req.length(), header, *m_device);
        hal::v5::write_and_flush(
          *m_ctrl_ep,
          scatter_span<byte const>(scatter_arr_pair.spans)
            .first(scatter_arr_pair.count));

        break;
      }

      case descriptor_type::configuration: {
        configuration& conf = m_configs->at(desc_idx);
        auto conf_hdr =
          std::to_array({ constants::configuration_descriptor_size,
                          static_cast<byte>(descriptor_type::configuration) });
        auto scatter_conf_pair = make_sub_scatter_bytes(
          req.length(), conf_hdr, static_cast<std::span<u8 const>>(conf));

        m_ctrl_ep->write(scatter_span<byte const>(scatter_conf_pair.spans)
                           .first(scatter_conf_pair.count));

        // Return early if the only thing requested was the config descriptor
        if (req.length() <= constants::configuration_descriptor_size) {
          m_ctrl_ep->write({});
          return;
        }

        u16 total_size = constants::configuration_descriptor_size;
        for (auto const& iface : conf.interfaces) {
          std::ignore = iface->write_descriptors(
            { .interface = std::nullopt, .string = std::nullopt },
            [this, &total_size](scatter_span<hal::byte const> byte_stream) {
              m_ctrl_ep->write(byte_stream);
              total_size += scatter_span_size(byte_stream);
            });
        }

        m_ctrl_ep->write({});
        // if (total_size != req.length()) {
        //   safe_throw(hal::operation_not_supported(
        //     this));  // TODO: Make specific exception for this
        // }

        break;
      }

      case descriptor_type::string: {
        if (desc_idx == 0) {

          auto s_hdr =
            std::to_array({ static_cast<byte>(4),
                            static_cast<byte>(descriptor_type::string) });
          auto lang = setup_packet::to_le_u16(m_lang_str);
          auto scatter_arr_pair = make_scatter_bytes(s_hdr, lang);
          // auto p = scatter_span<byte const>(scatter_arr_pair.spans)
          //            .first(scatter_arr_pair.count);
          m_ctrl_ep->write(scatter_arr_pair);
          m_ctrl_ep->write({});
          break;
        }
        handle_str_descriptors(desc_idx, req.length());  // Can throw
        break;
      }

        // TODO: Interface, endpoint, device_qualifier, interface_power,
        // OTHER_SPEED_CONFIGURATION

      default:
        safe_throw(hal::operation_not_supported(this));
    }
  }

  void handle_str_descriptors(u8 const target_idx, u16 p_len)
  {
    // Device strings at indexes 1-3, configuration strings start at 4
    constexpr u8 device_str_start = 1;
    u8 cfg_string_end = device_str_start + 3 + num_configs;
    if (target_idx <= cfg_string_end) {
      auto r = write_cfg_str_descriptor(target_idx, p_len);
      if (!r) {
        safe_throw(hal::argument_out_of_domain(this));
      }
      m_iface_for_str_desc = std::nullopt;
      return;
    }

    if (m_iface_for_str_desc.has_value() &&
        m_iface_for_str_desc->first == target_idx) {
      bool success = m_iface_for_str_desc->second->write_string_descriptor(
        target_idx, [this](scatter_span<hal::byte const> desc) {
          hal::v5::write_and_flush(*m_ctrl_ep, desc);
        });
      if (success) {
        return;
      }
    }

    // Iterate through every interface now to find a match
    auto f = [this, p_len](scatter_span<hal::byte const> desc) {
      if (p_len > 2) {
        hal::v5::write_and_flush(*m_ctrl_ep, desc);
      } else {
        std::array<hal::byte const, 1> desc_type{ static_cast<hal::byte>(
          descriptor_type::string) };
        auto scatter_str_hdr =
          make_scatter_bytes(std::span(&desc[0][0], 1), desc_type);
        hal::v5::write_and_flush(*m_ctrl_ep, scatter_str_hdr);
      }
    };

    if (m_active_conf != nullptr) {
      for (auto const& iface : m_active_conf->interfaces) {
        auto res = iface->write_string_descriptor(target_idx, f);
        if (res) {
          return;
        }
      }
    }

    for (configuration const& conf : *m_configs) {
      for (auto const& iface : conf.interfaces) {
        auto res = iface->write_string_descriptor(target_idx, f);
        if (res) {
          break;
        }
      }
    }
  }

  bool write_cfg_str_descriptor(u8 const target_idx, u16 p_len)
  {
    // Device strings are at fixed indexes: 1=manufacturer, 2=product, 3=serial
    constexpr u8 manufacturer_idx = 1;
    constexpr u8 product_idx = 2;
    constexpr u8 serial_number_idx = 3;
    constexpr u8 config_start_idx = 4;

    std::optional<std::u16string_view> opt_conf_sv;
    if (target_idx == manufacturer_idx) {
      opt_conf_sv = m_device->manufacturer_str;

    } else if (target_idx == product_idx) {
      opt_conf_sv = m_device->product_str;

    } else if (target_idx == serial_number_idx) {
      opt_conf_sv = m_device->serial_number_str;

    } else {
      for (size_t i = 0; i < m_configs->size(); i++) {
        configuration const& conf = m_configs->at(i);
        if (target_idx == (config_start_idx + i)) {
          opt_conf_sv = conf.name;
        }
      }
    }

    if (opt_conf_sv == std::nullopt) {
      return false;
    }

    // Acceptable to access without checking because guaranteed to be Some,
    // there is no pattern matching in C++ yet so unable to do this cleanly
    // (would require a check on every single one)

    auto sv = opt_conf_sv.value();
    std::mbstate_t state{};
    for (wchar_t const wc : sv) {
      std::array<char, 1024> tmp;
      size_t len = std::wcrtomb(tmp.data(), wc, &state);
      if (len == static_cast<size_t>(-1)) {

        continue;
      }

      for (size_t i = 0; i < len; i++) {
      }
    }

    auto const conf_sv_span = hal::as_bytes(opt_conf_sv.value());
    auto desc_len = static_cast<hal::byte>((conf_sv_span.size() + 2));

    auto hdr_arr = std::to_array(
      { desc_len, static_cast<hal::byte>(descriptor_type::string) });

    auto scatter_arr_pair =
      make_sub_scatter_bytes(p_len, hdr_arr, conf_sv_span);

    auto p = scatter_span<byte const>(scatter_arr_pair.spans)
               .first(scatter_arr_pair.count);
    hal::v5::write_and_flush(*m_ctrl_ep, p);

    return true;
  }

  strong_ptr<control_endpoint> m_ctrl_ep;
  strong_ptr<device> m_device;
  strong_ptr<std::array<configuration, num_configs>> m_configs;
  u16 m_lang_str;

  std::optional<std::pair<u8, strong_ptr<interface>>> m_iface_for_str_desc;
  configuration* m_active_conf = nullptr;
  bool m_has_setup_packet = false;
};
}  // namespace hal::v5::usb

namespace hal::usb {
using v5::usb::enumerator;
}
