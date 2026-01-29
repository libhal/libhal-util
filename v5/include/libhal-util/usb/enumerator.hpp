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

#include <array>
#include <cwchar>

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

namespace hal::usb {

/**
  TODO: (PhazonicRidley) Make class method for scatterspan class
  @brief Get the size of a scatter span, meaning how many elements is this
  scatterspan viewing.

  @param ss The scatter span to get the size of
  @return The number of elements ss is viewing
 */
template<typename T>
constexpr size_t scatter_span_size(hal::v5::scatter_span<T> ss)
{
  size_t res = 0;
  for (auto const& s : ss) {
    res += s.size();
  }

  return res;
}

/**
  TODO: (PhazonicRidley) Make class method for scatterspan class
  @brief Creates a scatter_span with p_spans and calculates how many internal
  spans are used for a given sub scatter_span starting at index zero and going
  to the p_count element.

  ```C++
  auto pair = make_scatter_span(n, ...);
  auto my_scatter_span = scatter_span(pair.first).first(pair.second)
  ```

  @tparam T The type all collections are to have of.
  @tparam Args... A packed set of collections of type T

  @param p_count How many elements (from the start) to take into the subspan
  @param p_spans The spans the scatter span is to view into

  @return A pair where the first element contains an array of spans used within
  the subscatter span, and the second element is the number of spans within
  the sub scatterspan
*/
template<typename T, typename... Args>
constexpr std::pair<std::array<std::span<T>, sizeof...(Args)>, size_t>
make_sub_scatter_array(size_t p_count, Args&&... p_spans)
{
  std::array<std::span<T>, sizeof...(Args)> full_ss{ std::span<T>(
    std::forward<Args>(p_spans))... };

  size_t total_span_len = scatter_span_size(scatter_span<T>(full_ss));
  std::array<std::span<T>, sizeof...(Args)> res;
  std::array<size_t, sizeof...(Args)> lens{ std::span<T>(p_spans).size()... };

  if (total_span_len <= p_count) {
    return std::make_pair(full_ss, full_ss.size());
  }
  size_t cur_len = 0;
  size_t i = 0;
  for (; i < lens.size(); i++) {
    auto l = lens[i];

    if (p_count >= (cur_len + l)) {
      res[i] = full_ss[i];
      cur_len += l;
      continue;
    }

    if (cur_len >= p_count) {
      return std::make_pair(res, i);
    }

    auto delta = p_count - cur_len;
    std::span<T> subspan = std::span(full_ss[i]).first(delta);
    res[i] = subspan;
    break;
  }

  return std::make_pair(res, i + 1);
}

/**
  TODO: (PhazonicRidley) Make class method for scatterspan class
  @brief Create a sub scatter span array from spans of bytes

  @tparam Args... A packed set of collections of bytes

  @param p_count How many elements (from the start) to take into the subspan
  @param p_spans The spans the scatter span is to view into

  @return A pair where the first element contains an array of spans used within
  the subscatter span, and the second element is the number of bytes within
  the sub scatterspan
*/
template<typename... Args>
constexpr std::pair<std::array<std::span<byte const>, sizeof...(Args)>, size_t>
make_sub_scatter_bytes(size_t p_count, Args&&... p_spans)
{
  return make_sub_scatter_array<byte const>(p_count,
                                            std::forward<Args>(p_spans)...);
}

template<size_t num_configs>
class enumerator
{

public:
  enumerator(
    strong_ptr<control_endpoint> const& p_ctrl_ep,
    strong_ptr<device> const& p_device,
    strong_ptr<std::array<configuration, num_configs>> const& p_configs,
    u16 p_lang_str,  // NOLINT
    u8 p_starting_str_idx,
    bool enumerate_immediately = true)
    : m_ctrl_ep(p_ctrl_ep)
    , m_device(p_device)
    , m_configs(p_configs)
    , m_lang_str(p_lang_str)
  {
    // Verify there is space to actually allocate indexes for configuration
    // Three string indexes are reserved for the device descriptor, then each
    // configuration has a name which reserves a string index strings and
    // Index 0 is reserved for the lang string
    if (p_starting_str_idx < 1 || p_starting_str_idx > 0xFF - 3 + num_configs) {
      safe_throw(hal::argument_out_of_domain(this));
    }
    m_starting_str_idx = p_starting_str_idx;

    if (enumerate_immediately) {
      enumerate();
    }
  }

  void enumerate()
  {
    // Renumerate, a config will only be set if
    if (m_active_conf != nullptr) {
      m_active_conf = nullptr;
      m_ctrl_ep->connect(false);
    }

    auto cur_str_idx = m_starting_str_idx;
    byte cur_iface_idx = 0;
    // Phase one: Preperation

    // Device
    m_device->manufacturer_index() = cur_str_idx++;
    m_device->product_index() = cur_str_idx++;
    m_device->serial_number_index() = cur_str_idx++;
    m_device->num_configurations() = num_configs;

    // Configurations
    for (size_t i = 0; i < num_configs; i++) {
      configuration& config = m_configs->at(i);
      config.configuration_index() = cur_str_idx++;
      config.configuration_value() = i + 1;
    }

    for (configuration& config : *m_configs) {
      auto total_length = static_cast<u16>(constants::config_desc_size);
      for (auto const& iface : config.interfaces) {
        interface::descriptor_count deltas = iface->write_descriptors(
          { .interface = cur_iface_idx, .string = cur_str_idx },
          [&total_length](scatter_span<hal::byte const> p_data) {
            total_length += scatter_span_size(p_data);
          });

        cur_iface_idx += deltas.interface;
        cur_str_idx += deltas.string;
      }
      config.num_interfaces() = cur_iface_idx;
      config.total_length() = total_length;
    }

    // Phase two: Writing

    // TODO: Make async
    bool finished_enumeration = false;
    bool volatile waiting_for_data = true;

    using on_receive_tag = control_endpoint::on_receive_tag;

    m_ctrl_ep->on_receive(
      [&waiting_for_data](on_receive_tag) { waiting_for_data = false; });
    m_ctrl_ep->connect(true);

    std::array<hal::byte, constants::size_std_req> raw_req;
    do {
      // TODO: Seriously, make this async
      while (waiting_for_data) {
        continue;
      }
      waiting_for_data = true;

      auto scatter_raw_req = make_writable_scatter_bytes(raw_req);
      auto num_bytes_read = m_ctrl_ep->read(scatter_raw_req);

      if (num_bytes_read == 0) {
        continue;
      }

      if (num_bytes_read != constants::size_std_req) {

        safe_throw(hal::message_size(num_bytes_read, this));
      }

      auto req = setup_packet(raw_req);

      if (req.get_recipient() != setup_packet::recipient::device) {
        safe_throw(hal::not_connected(this));
      }

      // ZLP is handled at write site
      handle_standard_device_request(req);
      if (static_cast<standard_request_types>(req.request) ==
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

    std::array<byte, 8> read_buf;
    auto scatter_read_buf = make_writable_scatter_bytes(read_buf);
    auto bytes_read = m_ctrl_ep->read(scatter_read_buf);
    std::span payload(read_buf.data(), bytes_read);

    setup_packet req(payload);
    if (determine_standard_request(req) == standard_request_types::invalid) {
      return;
    }

    if (determine_standard_request(req) ==
          standard_request_types::get_descriptor &&
        static_cast<descriptor_type>((req.value & 0xFF << 8) >> 8) ==
          descriptor_type::string) {
      handle_str_descriptors(req.value & 0xFF, req.length > 2);

    } else if (req.get_recipient() == setup_packet::recipient::device) {
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
          std::ignore = m_ctrl_ep->read(resp);
        };
      }
      bool req_handled = false;
      for (auto const& iface : get_active_configuration()) {
        req_handled = iface->handle_request(
          req.request_type, req.request, req.value, req.index, req.length, f);
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
        m_ctrl_ep->set_address(req.value);

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
        auto scatter_conf = make_scatter_bytes(
          std::span(&m_active_conf->configuration_value(), 1));
        m_ctrl_ep->write(scatter_conf);
        break;
      }

      case standard_request_types::set_configuration: {
        m_active_conf = &(m_configs->at(req.value - 1));
        break;
      }

      case standard_request_types::invalid:
      default:
        safe_throw(hal::not_connected(this));
    }
  }

  void process_get_descriptor(setup_packet& req)
  {
    hal::byte desc_type = (req.value & 0xFF << 8) >> 8;
    [[maybe_unused]] hal::byte desc_idx = req.value & 0xFF;

    switch (static_cast<descriptor_type>(desc_type)) {
      case descriptor_type::device: {
        auto header =
          std::to_array({ constants::device_desc_size,
                          static_cast<byte>(descriptor_type::device) });
        m_device->max_packet_size() = static_cast<byte>(m_ctrl_ep->info().size);
        auto scatter_arr_pair =
          make_sub_scatter_bytes(req.length, header, *m_device);
        hal::v5::write_and_flush(
          *m_ctrl_ep,
          scatter_span<byte const>(scatter_arr_pair.first)
            .first(scatter_arr_pair.second));

        break;
      }

      case descriptor_type::configuration: {
        configuration& conf = m_configs->at(desc_idx);
        auto conf_hdr =
          std::to_array({ constants::config_desc_size,
                          static_cast<byte>(descriptor_type::configuration) });
        auto scatter_conf_pair = make_sub_scatter_bytes(
          req.length, conf_hdr, static_cast<std::span<u8 const>>(conf));

        m_ctrl_ep->write(scatter_span<byte const>(scatter_conf_pair.first)
                           .first(scatter_conf_pair.second));

        // Return early if the only thing requested was the config descriptor
        if (req.length <= constants::config_desc_size) {
          m_ctrl_ep->write({});
          return;
        }

        u16 total_size = constants::config_desc_size;
        for (auto const& iface : conf.interfaces) {
          std::ignore = iface->write_descriptors(
            { .interface = std::nullopt, .string = std::nullopt },
            [this, &total_size](scatter_span<hal::byte const> byte_stream) {
              m_ctrl_ep->write(byte_stream);
              total_size += scatter_span_size(byte_stream);
            });
        }

        m_ctrl_ep->write({});
        break;
      }

      case descriptor_type::string: {
        if (desc_idx == 0) {

          auto s_hdr =
            std::to_array({ static_cast<byte>(4),
                            static_cast<byte>(descriptor_type::string) });
          auto lang = setup_packet::to_le_bytes(m_lang_str);
          auto scatter_arr_pair = make_scatter_bytes(s_hdr, lang);
          // auto p = scatter_span<byte const>(scatter_arr_pair.first)
          //            .first(scatter_arr_pair.second);
          m_ctrl_ep->write(scatter_arr_pair);
          m_ctrl_ep->write({});
          break;
        }
        handle_str_descriptors(desc_idx, req.length);  // Can throw
        break;
      }

      default:
        safe_throw(hal::operation_not_supported(this));
    }
  }

  void handle_str_descriptors(u8 const target_idx, u16 p_len)
  {

    u8 cfg_string_end = m_starting_str_idx + 3 + num_configs;
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
    constexpr u8 dev_manu_offset = 0;
    constexpr u8 dev_prod_offset = 1;
    constexpr u8 dev_sn_offset = 2;
    std::optional<std::u16string_view> opt_conf_sv;
    if (target_idx == (m_starting_str_idx + dev_manu_offset)) {
      opt_conf_sv = m_device->manufacturer_str;

    } else if (target_idx == (m_starting_str_idx + dev_prod_offset)) {
      opt_conf_sv = m_device->product_str;

    } else if (target_idx == (m_starting_str_idx + dev_sn_offset)) {
      opt_conf_sv = m_device->serial_number_str;

    } else {
      for (size_t i = 0; i < m_configs->size(); i++) {
        configuration const& conf = m_configs->at(i);
        if (target_idx == (m_starting_str_idx + i)) {
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

    auto p = scatter_span<byte const>(scatter_arr_pair.first)
               .first(scatter_arr_pair.second);
    hal::v5::write_and_flush(*m_ctrl_ep, p);

    return true;
  }

  strong_ptr<control_endpoint> m_ctrl_ep;
  strong_ptr<device> m_device;
  strong_ptr<std::array<configuration, num_configs>> m_configs;
  u16 m_lang_str;
  u8 m_starting_str_idx;
  std::optional<std::pair<u8, strong_ptr<interface>>> m_iface_for_str_desc;
  configuration* m_active_conf = nullptr;
  bool m_has_setup_packet = false;
};
}  // namespace hal::usb
