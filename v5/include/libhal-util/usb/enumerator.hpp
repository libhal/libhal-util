// Copyright 2026 Madeline Schneider and the libhal contributors
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
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

#include "descriptors.hpp"
#include "libhal-util/as_bytes.hpp"
#include "libhal-util/scatter_span.hpp"
#include "libhal-util/usb/endpoints.hpp"
#include "utils.hpp"

namespace hal::v5::usb {

using hal::v5::make_sub_scatter_array;
using hal::v5::make_sub_scatter_bytes;
using hal::v5::scatter_span_size;
using hal::v5::sub_scatter_result;

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
    u8 retry_max;
  };

  enumerator(args p_args)
    : m_ctrl_ep(p_args.ctrl_ep)
    , m_device(p_args.device)
    , m_configs(p_args.configs)
    , m_lang_str(p_args.lang_str)
    , m_retry_max(p_args.retry_max)
  {
    m_ctrl_ep->on_receive([this](control_endpoint::on_receive_tag) {
      if (!this->m_ctrl_ep->has_setup().has_value()) {
        m_has_setup_packet =
          true;  // We will just assume the packet is a setup packet, enumerator
                 // checks packet structure already
        return;
      }

      m_has_setup_packet = this->m_ctrl_ep->has_setup().value();
    });
  }

  enumerator(enumerator&&) = delete;
  bool operator=(enumerator&&) = delete;

  void start_enumeration()
  {
    if (m_active_conf != nullptr) {
      m_active_conf = nullptr;
      m_ctrl_ep->connect(false);
      m_reinit_descriptors = true;
    }

    if (m_reinit_descriptors) {
      prepare_descriptors();
      m_reinit_descriptors = false;
    }
  }

  [[nodiscard]] std::optional<std::reference_wrapper<configuration>>
  get_active_configuration()
  {
    if (m_active_conf == nullptr) {
      return std::nullopt;
    }

    return std::make_optional(std::ref(*m_active_conf));
  }

  [[nodiscard]] bool is_enumerated() const
  {
    return m_active_conf != nullptr;
  }

  void process_ctrl_transfer()
  {
    if (!m_has_setup_packet) {
      return;
    }

    if (m_retry_counter >= m_retry_max) {
      m_retry_counter = 0;
      throw hal::io_error(this);
    }

    if (m_ctrl_ep->info().stalled) {
      m_ctrl_ep->stall(false);
    }
    m_has_setup_packet = false;

    setup_packet req;
    auto& read_buf = req.raw_request_bytes;

    auto scatter_read_buf = make_writable_scatter_bytes(read_buf);
    auto bytes_read = m_ctrl_ep->read(scatter_read_buf);
    if (bytes_read != 8) {
      return;  // STATUS OUT ZLP or malformed — not a SETUP packet
    }

    if (req.get_type() == setup_packet::request_type::invalid) {
      m_ctrl_ep->stall(true);
      m_retry_counter += 1;
      return;
    }
    if (determine_standard_request(req) ==
          standard_request_types::get_descriptor &&
        static_cast<descriptor_type>(req.value_bytes()[1]) ==
          descriptor_type::string) {
      handle_str_descriptors(req.value_bytes()[0], req.length());

    } else if (req.get_recipient() == setup_packet::request_recipient::device) {
      try {
        // Developers are responsible for handling all other errors such as
        // arguments out of domain as thats user defined data.
        handle_standard_device_request(req);
      } catch (hal::operation_not_supported&) {
        m_ctrl_ep->stall(true);
        return;
      }

      if (determine_standard_request(req) ==
          standard_request_types::set_address) {
        return;
      }

    } else {
      // Handle iface level requests
      bool req_handled = false;
      std::optional<std::reference_wrapper<configuration>> active_conf =
        get_active_configuration();
      if (!active_conf.has_value()) {
        m_ctrl_ep->stall(true);
        m_retry_counter += 1;
        return;
      }

      try {
        enumerator_eio eio(*this);
        for (auto const& iface : active_conf.value().get().interfaces()) {
          req_handled = iface->handle_request(req, eio);
          if (req_handled) {
            break;
          }
        }
        // Handle driver exceptions if any, but make sure to inject a STALL in
        // first
      } catch (hal::exception& e) {
        m_ctrl_ep->stall(true);
        throw;
      }

      if (!req_handled) {
        m_ctrl_ep->stall(true);
        m_retry_counter += 1;
        return;
      }
    }

    m_ctrl_ep->write({});  // ZLP
  }

private:
  class enumerator_eio : endpoint_io
  {

  public:
    enumerator_eio() = delete;

  private:
    friend class enumerator;

    enumerator_eio(enumerator& p_en)
      : m_ctrl_ep(p_en.m_ctrl_ep)
    {
    }

    usize driver_read(scatter_span<byte> p_buffer) override
    {
      auto read = m_ctrl_ep->read(p_buffer);
      return read;
    }

    usize driver_write(scatter_span<byte const> p_buffer) override
    {
      // FIXME: Change write to return how many bytes written
      m_ctrl_ep->write(p_buffer);
      return scatter_span_size(p_buffer);
    }

    strong_ptr<control_endpoint> m_ctrl_ep;
  };

  struct size_eio : endpoint_io
  {

    usize driver_read(scatter_span<byte>) override
    {
      return 0;
    }

    usize driver_write(scatter_span<byte const> p_buffer) override
    {
      auto s = scatter_span_size(p_buffer);
      total_length += s;
      return s;
    }

    usize total_length = 0;
  };

  void prepare_descriptors()
  {
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

    size_eio seio;
    for (configuration& config : *m_configs) {
      auto total_length =
        static_cast<u16>(constants::configuration_descriptor_size);
      for (auto const& iface : config.m_interfaces) {
        auto deltas = iface->write_descriptors(
          { .interface = cur_iface_idx, .string = cur_str_idx }, seio);

        cur_iface_idx += deltas.interface;
        cur_str_idx += deltas.string;
      }
      config.set_num_interfaces(cur_iface_idx);
      total_length += seio.total_length;
      config.set_total_length(total_length);
      seio.total_length = 0;
    }

    m_ctrl_ep->connect(true);
  }
  u16 assemble_le_u16(std::span<byte const> bytes)
  {
    if (bytes.size() != 2) {
      safe_throw(hal::argument_out_of_domain(this));
    }

    return static_cast<u16>(bytes[0]) | (static_cast<u16>(bytes[1]) << 8);
  }

  void handle_standard_device_request(setup_packet& req)
  {
    auto det = determine_standard_request(req);
    switch (det) {
      case standard_request_types::set_address: {
        m_ctrl_ep->write({});  // ZLP must precede the address change
        m_ctrl_ep->set_address(req.value_bytes()[0]);
        return;
      }

      case standard_request_types::get_descriptor: {
        process_get_descriptor(req);
        break;
      }

      case standard_request_types::get_configuration: {
        if (m_active_conf == nullptr) {
          safe_throw(hal::operation_not_supported(this));
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
        safe_throw(hal::operation_not_supported(this));
    }
  }

  void process_get_descriptor(setup_packet& req)
  {
    auto const desc_type = static_cast<descriptor_type>(req.value_bytes()[1]);
    auto desc_idx = req.value_bytes()[0];
    enumerator_eio eio(*this);
    switch (desc_type) {
      case descriptor_type::device: {
        auto header =
          std::to_array({ constants::device_descriptor_size,
                          static_cast<byte>(descriptor_type::device) });
        m_device->set_max_packet_size(
          static_cast<byte>(m_ctrl_ep->info().size));
        auto scatter_arr_pair = make_sub_scatter_bytes(
          assemble_le_u16(req.length_bytes()), header, *m_device);
        hal::v5::write(*m_ctrl_ep,
                       scatter_span<byte const>(scatter_arr_pair.spans)
                         .first(scatter_arr_pair.count));
        break;
      }

      case descriptor_type::configuration: {
        configuration& conf = m_configs->at(desc_idx);
        auto conf_hdr =
          std::to_array({ constants::configuration_descriptor_size,
                          static_cast<byte>(descriptor_type::configuration) });
        auto scatter_conf_pair =
          make_sub_scatter_bytes(assemble_le_u16(req.length_bytes()),
                                 conf_hdr,
                                 static_cast<std::span<u8 const>>(conf));

        m_ctrl_ep->write(scatter_span<byte const>(scatter_conf_pair.spans)
                           .first(scatter_conf_pair.count));

        // TODO(#99): `enumerator_eio` should limit the amount written to the
        // control endpoint based on `request.length()` value.
        // Return early if the only thing requested was the config descriptor
        if (req.length() <= constants::configuration_descriptor_size) {
          return;
        }

        for (auto const& iface : conf.m_interfaces) {
          std::ignore = iface->write_descriptors(
            { .interface = std::nullopt, .string = std::nullopt }, eio);
        }
        break;
      }

      case descriptor_type::string: {
        if (desc_idx == 0) {

          auto s_hdr =
            std::to_array({ static_cast<byte>(4),
                            static_cast<byte>(descriptor_type::string) });
          auto lang = setup_packet::to_le_u16(m_lang_str);
          auto scatter_arr_pair = make_scatter_bytes(s_hdr, lang);
          m_ctrl_ep->write(scatter_arr_pair);
          break;
        }
        handle_str_descriptors(desc_idx, req.length());  // Can throw
        break;
      }

        // TODO(#95): device_qualifier
        // TODO(#96): OTHER_SPEED_CONFIGURATION

      default:
        safe_throw(hal::operation_not_supported(this));
    }
  }

  void handle_str_descriptors(u8 target_idx, u16 p_len)
  {
    // Device strings at indexes 1-3, configuration strings start at 4
    u8 cfg_string_end = num_configs + 3;
    enumerator_eio eio(*this);
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
      bool success =
        m_iface_for_str_desc->second->write_string_descriptor(target_idx, eio);
      if (success) {
        return;
      }
    }

    if (m_active_conf != nullptr) {
      for (auto const& iface : m_active_conf->m_interfaces) {
        auto res = iface->write_string_descriptor(target_idx, eio);
        if (res) {
          return;
        }
      }
    }

    for (configuration const& conf : *m_configs) {
      for (auto const& iface : conf.m_interfaces) {
        auto res = iface->write_string_descriptor(target_idx, eio);
        if (res) {
          break;
        }
      }
    }
  }

  bool write_cfg_str_descriptor(u8 const target_idx, u16 le_len)
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

    auto const conf_sv_span = hal::as_bytes(opt_conf_sv.value());
    auto desc_len = static_cast<hal::byte>((conf_sv_span.size() + 2));

    auto hdr_arr = std::to_array(
      { desc_len, static_cast<hal::byte>(descriptor_type::string) });

    auto scatter_arr_pair =
      make_sub_scatter_bytes(le_len, hdr_arr, conf_sv_span);

    auto p = scatter_span<byte const>(scatter_arr_pair.spans)
               .first(scatter_arr_pair.count);
    hal::v5::write(*m_ctrl_ep, p);

    return true;
  }

  strong_ptr<control_endpoint> m_ctrl_ep;
  strong_ptr<device> m_device;
  strong_ptr<std::array<configuration, num_configs>> m_configs;
  u16 m_lang_str;

  std::optional<std::pair<u8, strong_ptr<interface>>> m_iface_for_str_desc;
  configuration* m_active_conf = nullptr;
  bool volatile m_has_setup_packet = false;
  bool m_reinit_descriptors = true;
  u8 m_retry_counter = 0;
  u8 const m_retry_max;
};
}  // namespace hal::v5::usb

namespace hal::usb {
using v5::usb::enumerator;
}
