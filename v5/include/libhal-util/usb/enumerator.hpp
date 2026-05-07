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

#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

#include "../as_bytes.hpp"
#include "../scatter_span.hpp"
#include "../usb/endpoints.hpp"
#include "descriptors.hpp"
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
    m_ctrl_ep->on_host_event([this](v5::usb::host_event p_event) {
      // Assign
      m_event = p_event;
    });
  }

  enumerator(enumerator&&) = delete;
  bool operator=(enumerator&&) = delete;

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
    if (m_retry_counter >= m_retry_max) {
      m_retry_counter = 0;
      throw hal::io_error(this);
    }

    if (not m_event) {
      return;
    }

    switch (*m_event) {
      case v5::usb::host_event::reset:
        m_active_conf = nullptr;
        prepare_descriptors();
        m_ctrl_ep->connect(true);
        break;
      case v5::usb::host_event::setup_packet:
        handle_setup_packet();
        break;
      case v5::usb::host_event::data_packet:
        // Not sure what to do in this case as it shouldn't happen...
        break;
      default:  // The rest...
        pass_host_event_to_interfaces();
        break;
    }

    m_event.reset();
  }

private:
  void pass_host_event_to_interfaces()
  {
    if (m_active_conf == nullptr) {
      return;
    }
    for (auto& interface : m_active_conf->interfaces()) {
      interface->handle_host_event(*m_event);
    }
  }
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

  void handle_setup_packet()
  {
    if (m_ctrl_ep->info().stalled) {
      send_error_to_host();
    }

    setup_packet req;
    auto& read_buf = req.raw_request_bytes;
    auto scatter_read_buf = make_writable_scatter_bytes(read_buf);
    auto bytes_read = m_ctrl_ep->read(scatter_read_buf);

    if (bytes_read != 8) {
      // STATUS OUT ZLP or malformed - not a SETUP packet, STALL
      send_error_to_host();
      return;
    }

    switch (req.get_recipient()) {
      case setup_packet::request_recipient::invalid:
        send_error_to_host();
        break;

      case setup_packet::request_recipient::device:
        handle_standard_device_request(req);
        break;

      case setup_packet::request_recipient::interface:
        [[fallthrough]];
      case setup_packet::request_recipient::endpoint: {
        handle_interface_request(req);
      }
    }
  }

  void handle_interface_request(setup_packet p_request)
  {
    if (m_active_conf == nullptr) {
      send_error_to_host();
      return;
    }

    enumerator_eio eio(*this);
    bool request_handled = false;

    for (auto const& iface : m_active_conf->interfaces()) {
      request_handled = iface->handle_request(p_request, eio);
      if (request_handled) {
        break;
      }
    }

    if (!request_handled) {
      send_error_to_host();
    }

    m_ctrl_ep->write({});
  }

  void send_error_to_host()
  {
    m_ctrl_ep->stall(true);
    m_retry_counter += 1;
  }

  void prepare_descriptors()
  {
    // String indexes 1-3 are reserved for device descriptor strings
    // (manufacturer, product, serial number). Configuration strings start at 4.
    u8 cur_str_idx = 4;
    byte cur_iface_idx = 0;
    // Phase one: Preparation

    // Device
    m_device->set_num_configurations(num_configs);

    // Configurations
    {
      // Configuration index 0 is reserved/invalid
      // Configuration index value must be greater than 0.
      usize index = 1;
      for (configuration& config : *m_configs) {
        config.set_configuration_string_index(cur_str_idx++);
        config.set_configuration_value(index++);
      }
    }

    size_eio size_counting_endpoint;
    for (configuration& config : *m_configs) {
      auto total_length =
        static_cast<u16>(constants::configuration_descriptor_size);
      for (auto const& iface : config.m_interfaces) {
        auto deltas = iface->write_descriptors(
          { .interface = cur_iface_idx, .string = cur_str_idx },
          size_counting_endpoint);

        cur_iface_idx += deltas.interface;
        cur_str_idx += deltas.string;
      }
      config.set_num_interfaces(cur_iface_idx);
      total_length += size_counting_endpoint.total_length;
      config.set_total_length(total_length);
      size_counting_endpoint.total_length = 0;
    }
  }

  void handle_standard_device_request(setup_packet& req)
  {
    auto const request = determine_standard_request(req);
    switch (request) {
      case standard_request_types::set_address: {
        m_ctrl_ep->write({});  // ZLP must precede the address change
        m_ctrl_ep->set_address(req.value_bytes()[0]);
        return;
      }

      case standard_request_types::get_descriptor: {
        send_requested_descriptor(req);
        break;
      }

      case standard_request_types::get_configuration: {
        if (m_active_conf == nullptr) {
          send_error_to_host();
          return;
        }
        auto conf_value = m_active_conf->configuration_value();
        auto scatter_conf = make_scatter_bytes(std::span(&conf_value, 1));
        hal::v5::write_and_flush(*m_ctrl_ep, scatter_conf);
        break;
      }

      case standard_request_types::set_configuration: {
        m_active_conf = &(m_configs->at(req.value() - 1));
        m_ctrl_ep->write({});
        break;
      }

      case standard_request_types::invalid:
        [[fallthrough]];
      default:
        send_error_to_host();
    }
  }

  void send_requested_descriptor(setup_packet& req)
  {
    auto const type = static_cast<descriptor_type>(req.value_bytes()[1]);
    auto const index = req.value_bytes()[0];

    switch (type) {
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
        if (m_configs->empty()) {
          send_error_to_host();
          return;
        }

        configuration& conf = (*m_configs)[0];
        auto const conf_hdr =
          std::to_array({ constants::configuration_descriptor_size,
                          static_cast<byte>(descriptor_type::configuration) });
        auto scatter_conf_pair = make_sub_scatter_bytes(
          req.length(), conf_hdr, static_cast<std::span<u8 const>>(conf));

        m_ctrl_ep->write(scatter_span<byte const>(scatter_conf_pair.spans)
                           .first(scatter_conf_pair.count));

        // TODO(#99): `enumerator_eio` should limit the amount written to the
        // control endpoint based on `request.length()` value.
        // Return early if the only thing requested was the config descriptor
        if (req.length() <= constants::configuration_descriptor_size) {
          m_ctrl_ep->write({});  // ZLP to finish
          return;
        }

        {
          enumerator_eio eio(*this);
          for (auto const& iface : conf.m_interfaces) {
            std::ignore = iface->write_descriptors(
              { .interface = std::nullopt, .string = std::nullopt }, eio);
          }
        }

        m_ctrl_ep->write({});  // ZLP to finish
        break;
      }

      case descriptor_type::string: {
        handle_str_descriptors(index, req.length());
        m_ctrl_ep->write({});  // ZLP to finish
        break;
      }

        // TODO(#95): device_qualifier
        // TODO(#96): OTHER_SPEED_CONFIGURATION

      default: {
        send_error_to_host();
      }
    }
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void handle_str_descriptors(u8 p_target, u16 p_len)
  {
    // Device strings are at fixed indexes: 1=manufacturer, 2=product, 3=serial
    constexpr u8 language_id = 0;
    constexpr u8 manufacturer_idx = 1;
    constexpr u8 product_idx = 2;
    constexpr u8 serial_number_idx = 3;

    switch (p_target) {
      case language_id: {
        auto const payload = std::to_array<hal::byte const>({
          static_cast<byte>(4),                         // length
          static_cast<byte>(descriptor_type::string),   // type
          static_cast<byte>(m_lang_str & 0xFF),         // LE0
          static_cast<byte>((m_lang_str >> 8) & 0xFF),  // LE1
        });
        auto const final_payload = hal::v5::make_scatter_array<byte const>(
          std::span(payload).first(std::min<usize>(p_len, payload.size())));
        m_ctrl_ep->write(final_payload);
        break;
      }
      case manufacturer_idx: {
        write_string_view(m_device->manufacturer_str, p_len);
        break;
      }
      case product_idx: {
        write_string_view(m_device->product_str, p_len);
        break;
      }
      case serial_number_idx: {
        write_string_view(m_device->serial_number_str, p_len);
        break;
      }
      default: {
        // Check if the index is for the configuration string
        for (configuration const& conf : *m_configs) {
          if (p_target == conf.configuration_string_index()) {
            write_string_view(conf.name, p_len);
            break;
          }
        }

        enumerator_eio endpoint(*this);

        // Check if this index belongs to any interfaces
        if (m_active_conf != nullptr) {
          for (auto const& iface : m_active_conf->m_interfaces) {
            if (iface->write_string_descriptor(p_target, endpoint) == true) {
              return;
            }
          }
        }

        // Check across all configurations
        for (configuration const& conf : *m_configs) {
          for (auto const& iface : conf.m_interfaces) {
            if (iface->write_string_descriptor(p_target, endpoint)) {
              return;
            }
          }
        }
        break;
      }
    }
  }

  void write_string_view(std::u16string_view p_str, u16 p_max_length)
  {
    auto const string_view_as_bytes = hal::as_bytes(p_str);
    auto const length =
      static_cast<hal::byte>((string_view_as_bytes.size() + 2));

    auto const header = std::to_array(
      { length, static_cast<hal::byte>(descriptor_type::string) });

    auto const scatter_arr_pair =
      make_sub_scatter_bytes(p_max_length, header, string_view_as_bytes);

    auto const p = scatter_span<byte const>(scatter_arr_pair.spans)
                     .first(scatter_arr_pair.count);
    hal::v5::write(*m_ctrl_ep, p);
  }

  strong_ptr<control_endpoint> m_ctrl_ep;
  strong_ptr<device> m_device;
  strong_ptr<std::array<configuration, num_configs>> m_configs;
  u16 m_lang_str;
  u8 m_retry_max;

  std::optional<v5::usb::host_event> m_event = v5::usb::host_event::reset;
  configuration* m_active_conf = nullptr;
  u8 m_retry_counter = 0;
};
}  // namespace hal::v5::usb

namespace hal::usb {
using v5::usb::enumerator;
}
