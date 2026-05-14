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

#include <array>
#include <initializer_list>
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
#include "constants.hpp"

namespace hal::v5::usb {

using hal::v5::make_sub_scatter_array;
using hal::v5::make_sub_scatter_bytes;
using hal::v5::scatter_span_size;
using hal::v5::sub_scatter_result;

class enumerator
{
public:
  struct info
  {
    // ---- Required: no sensible defaults ----
    /// Manufacturing name for this USB device
    std::u16string_view manufacturer;
    /// Name of the USB product
    std::u16string_view product;
    /// Serial number of this USB device
    std::u16string_view serial_number;
    /// 16-bit Vendor ID
    u16 vendor_id;
    /// 16-bit Product ID
    u16 product_id;

    // ---- Optional: reasonable defaults provided ----

    /// USB specification version. 0x0201 minimum for WebUSB/BOS support.
    u16 usb_version = 0x0200;

    /// Maximum bus current drawn in milli-amps. Stored as mA, converted to
    /// 2mA units when writing the configuration descriptor.
    u16 max_power_mA = 100;

    /// Language ID for string descriptors. 0x0409 = English (US).
    u16 lang_id = 0x0409;

    /// Firmware/hardware version presented to the host.
    u16 device_version = 0x0100;

    u8 retry_max = 3;

    bool self_powered = false;
    bool remote_wakeup = false;
  };

  enumerator(strong_ptr<control_endpoint> const& p_ctrl_ep,
             info p_info,
             std::span<strong_ptr<interface>> p_interfaces)
    : m_ctrl_ep(p_ctrl_ep)
    , m_interfaces(p_interfaces)
    , m_info(p_info)
  {
    m_ctrl_ep->on_host_event(
      [this](v5::usb::bus_event p_event) { m_event = p_event; });
  }
  ~enumerator()
  {
    m_ctrl_ep->on_host_event([](v5::usb::bus_event) {});
  }

  enumerator(enumerator&&) = delete;
  bool operator=(enumerator&&) = delete;

  [[nodiscard]] bool is_enumerated() const
  {
    return m_enumerated;
  }

  void process_ctrl_transfer()
  {
    if (m_retry_counter >= m_info.retry_max) {
      m_retry_counter = 0;
      throw hal::io_error(this);
    }

    if (not m_event) {
      return;
    }

    auto event = *m_event;

    using bus_event = v5::usb::bus_event;

    switch (event) {
      case bus_event::reset:
        m_enumerated = false;
        m_ctrl_ep->connect(true);
        pass_host_event_to_interfaces(host_event::reset);
        break;
      case bus_event::setup_packet:
        handle_setup_packet();
        break;
      case bus_event::data_packet:
        break;
      case bus_event::resume:
        pass_host_event_to_interfaces(host_event::resume);
        break;
      case bus_event::suspend:
        if (m_ctrl_ep->remote_wakeup_granted()) {
          pass_host_event_to_interfaces(host_event::suspend_with_wakeup);
        }
        pass_host_event_to_interfaces(host_event::suspend_without_wakeup);
        break;
      case bus_event::sleep:
        pass_host_event_to_interfaces(host_event::sleep);
        break;
      case bus_event::u1_sleep:
        pass_host_event_to_interfaces(host_event::u1_sleep);
        break;
      case bus_event::u2_sleep:
        pass_host_event_to_interfaces(host_event::u2_sleep);
        break;
    }

    m_event.reset();
  }

private:
  void pass_host_event_to_interfaces(host_event p_event)
  {
    for (auto& interface : m_interfaces) {
      interface->handle_host_event(p_event);
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
      auto const read = m_ctrl_ep->read(p_buffer);
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
      auto const length = scatter_span_size(p_buffer);
      total_length += length;
      return length;
    }

    usize total_length = 0;
  };

  void handle_setup_packet()
  {
    setup_packet request;
    // Reset the m_event before the read. After read the data packet can come in
    // at any time, meaning we want this disengaged this function doesn't get
    // called
    m_event.reset();
    auto bytes_read =
      m_ctrl_ep->read(make_writable_scatter_bytes(request.raw_request_bytes));

    if (bytes_read != 8) {
      // STATUS OUT ZLP or malformed - not a SETUP packet, STALL
      send_error_to_host();
      return;
    }

    dispatch_setup_packet(request);
  }

  void dispatch_setup_packet(setup_packet& p_request)
  {
    switch (p_request.get_recipient()) {
      case setup_packet::request_recipient::invalid:
        send_error_to_host();
        break;

      case setup_packet::request_recipient::device:
        handle_standard_device_request(p_request);
        break;

      case setup_packet::request_recipient::interface:
        [[fallthrough]];
      case setup_packet::request_recipient::endpoint:
        handle_interface_request(p_request);
        break;
    }
  }

  void handle_interface_request(setup_packet p_request)
  {
    enumerator_eio eio(*this);
    bool request_handled = false;

    for (auto const& iface : m_interfaces) {
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

  /**
   * @brief Prepare interfaces with their starting interface and starting string
   * number.
   *
   * @return auto - total length of the interface descriptors
   */
  auto prepare_descriptors()
  {
    // String indexes 1-3 are reserved for device descriptor strings
    // (manufacturer, product, serial number). Configuration strings start at 4.
    u8 cur_str_index = 4;
    byte cur_iface_index = 0;

    // Phase one: Preparation
    size_eio size_counting_endpoint;

    for (auto const& iface : m_interfaces) {
      auto [interface_count, string_count] = iface->write_descriptors(
        {
          .interface = cur_iface_index,
          .string = cur_str_index,
        },
        size_counting_endpoint);

      cur_iface_index += interface_count;
      cur_str_index += string_count;
    }

    return size_counting_endpoint.total_length +
           static_cast<u16>(constants::configuration_descriptor_size);
  }

  void handle_standard_device_request(setup_packet& p_request)
  {
    enum struct feature : u8
    {
      device_remote_wakeup = 1,
      test_mode = 2
    };

    auto const request = determine_standard_request(p_request);
    switch (request) {
      case standard_request_types::set_address: {
        m_ctrl_ep->write({});  // ZLP must precede the address change
        m_ctrl_ep->set_address(p_request.value_bytes()[0]);
        return;
      }

      case standard_request_types::get_descriptor: {
        send_requested_descriptor(p_request);
        break;
      }

      case standard_request_types::get_configuration: {
        auto const payload = std::to_array<byte const, 1>({ 1 });
        auto const s_span = make_scatter_array<byte const>(payload);
        hal::v5::write_and_flush(*m_ctrl_ep, s_span);
        break;
      }

      case standard_request_types::set_configuration: {
        m_enumerated = true;
        pass_host_event_to_interfaces(hal::v5::usb::host_event::enumerated);
        m_ctrl_ep->write({});
        break;
      }

      case hal::v5::usb::standard_request_types::get_status: {
        byte const status = (m_info.self_powered << 0) |
                            (m_ctrl_ep->remote_wakeup_granted() << 1);
        auto const payload = std::to_array<byte const, 2>({ status, 0 });
        auto const s_span = make_scatter_array<byte const>(payload);
        hal::v5::write_and_flush(*m_ctrl_ep, s_span);
        break;
      }

      case standard_request_types::clear_feature: {
        auto const value = static_cast<feature>(p_request.value());
        if (value == feature::device_remote_wakeup) {
          m_ctrl_ep->remote_wakeup_enable(false);
          m_ctrl_ep->write({});
          break;
        }
        m_ctrl_ep->stall(true);
        break;
      }
      case standard_request_types::set_feature: {
        auto const value = static_cast<feature>(p_request.value());
        if (value == feature::device_remote_wakeup and m_info.remote_wakeup and
            m_ctrl_ep->supports_lpm().remote_wakeup_supported()) {
          m_ctrl_ep->remote_wakeup_enable(true);
          m_ctrl_ep->write({});
          break;
        }
        m_ctrl_ep->stall(true);
        break;
      }
      case standard_request_types::invalid:
      default:
        send_error_to_host();
    }
  }

  constexpr auto generate_device_descriptor()
  {
    static constexpr u8 b_length = 0;
    static constexpr u8 b_descriptor_type = 1;
    static constexpr u8 bcd_usb_lo = 2;
    static constexpr u8 bcd_usb_hi = 3;
    static constexpr u8 b_device_class = 4;
    static constexpr u8 b_device_sub_class = 5;
    static constexpr u8 b_device_protocol = 6;
    static constexpr u8 b_max_packet_size0 = 7;
    static constexpr u8 id_vendor_lo = 8;
    static constexpr u8 id_vendor_hi = 9;
    static constexpr u8 id_product_lo = 10;
    static constexpr u8 id_product_hi = 11;
    static constexpr u8 bcd_device_lo = 12;
    static constexpr u8 bcd_device_hi = 13;
    static constexpr u8 i_manufacturer = 14;
    static constexpr u8 i_product = 15;
    static constexpr u8 i_serial_number = 16;
    static constexpr u8 b_num_configurations = 17;

    std::array<byte, constants::device_descriptor_size> descriptor{};

    descriptor[b_length] = static_cast<byte>(descriptor.size());
    descriptor[b_descriptor_type] = 1;

    auto const bcd_usb = setup_packet::to_le_u16(m_info.usb_version);
    descriptor[bcd_usb_lo] = bcd_usb[0];
    descriptor[bcd_usb_hi] = bcd_usb[1];

    descriptor[b_device_class] = 0x00;
    descriptor[b_device_sub_class] = 0x00;
    descriptor[b_device_protocol] = 0x00;
    descriptor[b_max_packet_size0] = m_ctrl_ep->info().size;

    auto const id_vendor = setup_packet::to_le_u16(m_info.vendor_id);
    descriptor[id_vendor_lo] = id_vendor[0];
    descriptor[id_vendor_hi] = id_vendor[1];

    auto const id_product = setup_packet::to_le_u16(m_info.product_id);
    descriptor[id_product_lo] = id_product[0];
    descriptor[id_product_hi] = id_product[1];

    auto const bcd_device = setup_packet::to_le_u16(m_info.device_version);
    descriptor[bcd_device_lo] = bcd_device[0];
    descriptor[bcd_device_hi] = bcd_device[1];

    // Default string indexes, assuming enumeration will use 4 onward for string
    // indexes are these are required (can be modified by enumerator if desired)
    descriptor[i_manufacturer] = 1;
    descriptor[i_product] = 2;
    descriptor[i_serial_number] = 3;

    descriptor[b_num_configurations] = 1;

    return descriptor;
  }

  constexpr auto generate_configuration_descriptor(u16 p_total_length)
  {
    static constexpr u8 b_length = 0;
    static constexpr u8 b_descriptor_type = 1;
    static constexpr u8 w_total_length_lo = 2;
    static constexpr u8 w_total_length_hi = 3;
    static constexpr u8 b_num_interfaces = 4;
    static constexpr u8 b_configuration_value = 5;
    static constexpr u8 i_configuration = 6;
    static constexpr u8 bm_attributes = 7;
    static constexpr u8 b_max_power = 8;

    std::array<byte, constants::configuration_descriptor_size> descriptor{};

    descriptor[b_length] = static_cast<byte>(descriptor.size());
    descriptor[b_descriptor_type] = 2;

    auto const total_length = setup_packet::to_le_u16(p_total_length);
    descriptor[w_total_length_lo] = total_length[0];
    descriptor[w_total_length_hi] = total_length[1];

    descriptor[b_num_interfaces] = static_cast<byte>(m_interfaces.size());
    descriptor[b_configuration_value] = 1;
    descriptor[i_configuration] = 0;  // String index for configuration

    // Bit 7 is reserved and must be set; bits 6 and 5 are self-powered and
    // remote wakeup respectively.
    descriptor[bm_attributes] = static_cast<byte>(
      (1u << 7) | (m_info.self_powered << 6) | (m_info.remote_wakeup << 5));

    // bMaxPower is in 2mA units
    descriptor[b_max_power] = static_cast<byte>(m_info.max_power_mA / 2);

    return descriptor;
  }

  void send_requested_descriptor(setup_packet& p_request)
  {
    auto const type = static_cast<descriptor_type>(p_request.value_bytes()[1]);

    switch (type) {
      case descriptor_type::device: {
        auto const descriptor = generate_device_descriptor();
        auto const length =
          std::min<usize>(p_request.length(), descriptor.size());
        auto const payload = std::span(descriptor).first(length);
        hal::v5::write_and_flush(*m_ctrl_ep,
                                 make_scatter_array<byte const>(payload));
        break;
      }

      case descriptor_type::configuration: {
        auto const total_size = prepare_descriptors();
        auto const descriptor = generate_configuration_descriptor(total_size);
        auto const length =
          std::min<usize>(p_request.length(), descriptor.size());
        auto const payload = std::span(descriptor).first(length);
        hal::v5::write(*m_ctrl_ep, make_scatter_array<byte const>(payload));

        if (p_request.length() <= constants::configuration_descriptor_size) {
          m_ctrl_ep->write({});  // ZLP to finish
          return;
        }

        {
          // TODO(#99): `enumerator_eio` should limit the amount written to the
          // control endpoint based on `request.length()` value. When the limit
          // has been reached, the driver_write API of enumerator_eio should
          // simply do nothing and return early.
          enumerator_eio eio(*this);
          for (auto const& iface : m_interfaces) {
            std::ignore = iface->write_descriptors(
              { .interface = std::nullopt, .string = std::nullopt }, eio);
          }
        }

        m_ctrl_ep->write({});  // ZLP to finish
        break;
      }

      case descriptor_type::string: {
        handle_str_descriptors(p_request);
        break;
      }

        // TODO(#95): device_qualifier
        // TODO(#96): OTHER_SPEED_CONFIGURATION

      default: {
        send_error_to_host();
      }
    }
  }

  void handle_str_descriptors(setup_packet& p_request)
  {
    // Device strings are at fixed indexes: 1=manufacturer, 2=product, 3=serial
    constexpr u8 language_id = 0;
    constexpr u8 manufacturer_index = 1;
    constexpr u8 product_index = 2;
    constexpr u8 serial_number_index = 3;

    auto const target = p_request.value_bytes()[0];
    auto const length = p_request.length();

    switch (target) {
      case language_id: {
        auto const lang_id_le = setup_packet::to_le_u16(m_info.lang_id);
        auto const payload = std::to_array<hal::byte const>({
          byte{ 4 },                                   // length
          static_cast<byte>(descriptor_type::string),  // type
          lang_id_le[0],                               // LE0
          lang_id_le[1],                               // LE1
        });
        auto const payload_length = std::min<usize>(length, payload.size());
        auto const payload_span = std::span(payload).first(payload_length);
        auto const final_payload =
          hal::v5::make_scatter_array<byte const>(payload_span);
        hal::v5::write_and_flush(*m_ctrl_ep, final_payload);
        return;
      }
      case manufacturer_index: {
        write_string_view(m_info.manufacturer, length);
        return;
      }
      case product_index: {
        write_string_view(m_info.product, length);
        return;
      }
      case serial_number_index: {
        write_string_view(m_info.serial_number, length);
        return;
      }
      default: {
        enumerator_eio endpoint(*this);
        for (auto const& iface : m_interfaces) {
          if (iface->write_string_descriptor(target, endpoint) == true) {
            m_ctrl_ep->write({});
            return;
          }
        }
        break;
      }
    }
    send_error_to_host();
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

    auto const payload = scatter_span<byte const>(scatter_arr_pair.spans)
                           .first(scatter_arr_pair.count);

    hal::v5::write_and_flush(*m_ctrl_ep, payload);
  }

  strong_ptr<control_endpoint> m_ctrl_ep;
  std::span<strong_ptr<interface>> m_interfaces;
  std::optional<v5::usb::bus_event> m_event = v5::usb::bus_event::reset;
  info m_info;
  u8 m_retry_counter = 0;
  bool m_enumerated = false;
};

template<usize InterfaceCount>
class inplace_enumerator : public enumerator
{
public:
  template<typename... Interfaces>
  inplace_enumerator(strong_ptr<control_endpoint> const& p_ctrl_ep,
                     enumerator::info p_info,
                     Interfaces&&... p_interfaces)
    : enumerator(p_ctrl_ep, p_info, m_interface_storage)
    , m_interface_storage{ std::forward<Interfaces>(p_interfaces)... }
  {
  }

  inplace_enumerator() = delete;
  inplace_enumerator(inplace_enumerator const&) = delete;
  inplace_enumerator& operator=(inplace_enumerator const&) = delete;
  inplace_enumerator(inplace_enumerator&&) noexcept = default;
  inplace_enumerator& operator=(inplace_enumerator&&) noexcept = default;

private:
  std::array<strong_ptr<interface>, InterfaceCount> m_interface_storage;
};

template<typename... Interfaces>
inplace_enumerator(strong_ptr<control_endpoint> const&,
                   enumerator::info,
                   Interfaces&&...)
  -> inplace_enumerator<sizeof...(Interfaces)>;
}  // namespace hal::v5::usb

namespace hal::usb {
using v5::usb::enumerator;
using v5::usb::inplace_enumerator;
}  // namespace hal::usb
