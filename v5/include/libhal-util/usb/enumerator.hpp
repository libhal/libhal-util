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

/**
 * @brief USB Device Enumerator for handling USB protocol and descriptor
 * management.
 *
 * This class manages the full USB enumeration process, including device
 * identification, descriptor handling, and interface management. It acts as
 * the bridge between the hardware USB controller and the device's functional
 * interfaces, processing USB bus events and setup packets, generating device
 * responses, and delegating interface-specific work to registered interfaces.
 *
 * The enumerator is responsible for:
 *
 * - Processing USB bus events (reset, setup packets, suspend/resume)
 * - Managing device enumeration state
 * - Generating and sending device and configuration descriptors
 * - Handling standard USB requests (GET_DESCRIPTOR, SET_ADDRESS, etc.)
 * - Routing interface-specific requests to registered interfaces
 * - Forwarding host events to interfaces for state management
 *
 * This enumerator is one implementation option for managing USB device
 * enumeration. Applications are free to implement their own enumerator if this
 * one does not meet their needs.
 *
 * The enumerator takes shared ownership of the control endpoint and interfaces
 * via strong_ptr. Once objects are given to the enumerator, their virtual APIs
 * must not be called from outside the enumerator. The enumerator is solely
 * responsible for driving these APIs; external calls can violate critical
 * invariants about device state and enumeration progress.
 */
class enumerator
{
public:
  /**
   * @brief Configuration information for a USB device.
   *
   * This structure contains both required and optional fields for describing
   * the USB device to the host during enumeration. Required fields must be
   * provided at enumerator construction. Optional fields have sensible defaults
   * suitable for most USB 2.0 devices.
   *
   * All string fields (manufacturer, product, serial_number) must be UTF-16
   * encoded as they are sent directly to the host in string descriptors.
   */
  struct info
  {
    // ---- Required: no sensible defaults ----

    /**
     * @brief Manufacturing name for this USB device
     *
     * This UTF-16 string is sent as string descriptor index 1 during
     * enumeration. It identifies the manufacturer to the host and must not
     * be empty. Typically a company name like "Acme Corporation".
     */
    std::u16string_view manufacturer;

    /**
     * @brief Product name for this USB device
     *
     * This UTF-16 string is sent as string descriptor index 2 during
     * enumeration. It identifies the product to the host and must not be
     * empty. Typically a descriptive product name like "USB Serial Adapter".
     */
    std::u16string_view product;

    /**
     * @brief Serial number of this USB device
     *
     * This UTF-16 string is sent as string descriptor index 3 during
     * enumeration. It uniquely identifies individual units of the same product
     * to the host and must not be empty. Typically a numeric or alphanumeric
     * code assigned during manufacturing.
     */
    std::u16string_view serial_number;

    /**
     * @brief 16-bit USB Vendor ID
     *
     * The official Vendor ID assigned by the USB Implementers Forum (USB-IF).
     * This identifies the manufacturer and must be a valid, registered ID
     * obtained from the USB-IF. Common IDs for development/prototyping may
     * require explicit permission from the USB-IF.
     */
    u16 vendor_id;

    /**
     * @brief 16-bit USB Product ID
     *
     * The Product ID assigned by the manufacturer. Combined with vendor_id,
     * this uniquely identifies this product and its revision. The manufacturer
     * is responsible for assigning unique IDs within their vendor space.
     */
    u16 product_id;

    // ---- Optional: reasonable defaults provided ----

    /**
     * @brief USB specification version in binary-coded decimal format
     *
     * Indicates which USB specification version this device complies with.
     * Format is 0xJJMN where JJ is the major version and MN is the minor
     * version.
     *
     * Common values:
     * - 0x0200 (USB 2.0) — Default, suitable for most devices
     * - 0x0201 (USB 2.0 with enhancements) — Required for WebUSB and Binary
     *   Object Store (BOS) descriptor support
     * - 0x0300 (USB 3.0) — For SuperSpeed devices
     * - 0x0310 (USB 3.1) — For SuperSpeed+ devices
     *
     * Default: 0x0200
     */
    u16 usb_version = 0x0200;

    /**
     * @brief Maximum bus current drawn by this device in milliamps
     *
     * The maximum current the device draws from the USB bus, specified in mA.
     * The enumerator converts this to 2mA units when writing the configuration
     * descriptor, so this value is internally divided by 2 before transmission.
     *
     * The host uses this value to prevent overloading the USB power supply and
     * ensure sufficient current is available for the device. Most devices draw
     * between 50–500mA depending on their functionality.
     *
     * Bus-powered devices must not request more than 500mA. Self-powered
     * devices may request less to indicate minimal bus draw.
     *
     * Default: 500mA
     */
    u16 max_power_mA = 250;

    /**
     * @brief LANGID (Unicode Language ID) for string descriptors
     *
     * Specifies the primary language used in string descriptors. This value
     * is sent in the language ID string descriptor and allows the host to
     * select string descriptors in the appropriate language.
     *
     * Common values:
     * - 0x0409 (English - United States) — Default, widely supported
     * - 0x0809 (English - United Kingdom)
     * - 0x040C (French - France)
     * - 0x0407 (German - Germany)
     *
     * Default: 0x0409
     */
    u16 lang_id = 0x0409;

    /**
     * @brief Device firmware/hardware version in binary-coded decimal format
     *
     * A version number presented to the host to identify the device's
     * firmware and/or hardware revision. Format is 0xJJMN where JJ is the
     * major version and MN is the minor version.
     *
     * This allows the host to track which firmware/hardware revisions are
     * deployed and may be used by drivers to determine compatibility or
     * apply version-specific workarounds.
     *
     * Default: 0x0100 (version 1.0)
     */
    u16 device_version = 0x0100;

    /**
     * @brief Maximum number of control endpoint retries on protocol errors
     *
     * When the enumerator encounters certain USB protocol errors (such as
     * receiving a malformed setup packet), it stalls the control endpoint and
     * increments an internal retry counter. If the counter reaches this limit,
     * an `io_error` exception is thrown.
     *
     * This provides a safety mechanism to prevent infinite retry loops on
     * hardware or communication issues.
     *
     * Default: 3
     */
    u8 retry_max = 3;

    /**
     * @brief Whether this device is self-powered or bus-powered
     *
     * Set to true if the device draws some or all of its power from an
     * external source (not the USB bus). Set to false if the device draws
     * all of its power from the USB bus.
     *
     * This value is reported in the device status and configuration
     * descriptors. The host uses this information to manage power delivery
     * and display appropriate UI indications to the user.
     *
     * Default: false (bus-powered)
     */
    bool self_powered = false;

    /**
     * @brief Whether this device supports remote wakeup
     *
     * Set to true if the device hardware is capable of signaling the host to
     * wake from suspend state. The host may grant permission for remote wakeup
     * via the SET_FEATURE(DEVICE_REMOTE_WAKEUP) request during enumeration.
     *
     * Even if set to true, the host must grant permission before the device
     * can initiate wakeup. When remote wakeup is enabled by the host, writing
     * to an IN endpoint while the bus is suspended will automatically assert
     * resume signaling before completing the transfer.
     *
     * Default: false
     */
    bool remote_wakeup = false;
  };

  /**
   * @brief Construct a new USB device enumerator.
   *
   * Initializes the enumerator with the provided control endpoint, device
   * information, and interface list. This constructor also sets up a callback
   * on the control endpoint to capture and store incoming USB bus events.
   *
   * @param p_ctrl_ep A strong pointer to the device's control endpoint.
   * @param p_info Configuration information for the USB device.
   * @param p_interfaces A span of strong pointers to the device's interfaces.
   *                     The lifetime of the array backing this span must
   *                     outlive this enumerator object.
   */
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

  /**
   * @brief Check whether the device has completed USB enumeration.
   *
   * Returns whether the host has successfully completed the enumeration
   * process by sending a SET_CONFIGURATION request. Prior to enumeration,
   * the device can only respond to standard requests like GET_DESCRIPTOR
   * and SET_ADDRESS. After enumeration, the device's interfaces become
   * active and can process interface-specific requests.
   *
   * @return true if the device has been enumerated; false otherwise
   */
  [[nodiscard]] bool is_enumerated() const
  {
    return m_enumerated;
  }

  /**
   * @brief Process the current USB bus event captured by the enumerator.
   *
   * This function handles USB bus events (reset, setup packets, suspend,
   * resume, etc.) that have been captured by the control endpoint's host
   * event callback. It dispatches events appropriately: reset events trigger
   * device reconnection, setup packets are dispatched to the request handler,
   * and suspend/resume/sleep events are forwarded to registered interfaces.
   *
   * This function must be called regularly in the main application loop to
   * process incoming USB events. A retry counter tracks protocol errors; if
   * the counter reaches the configured maximum, an io_error is thrown as a
   * safety mechanism to prevent infinite error loops.
   *
   * @throws hal::io_error if the retry counter exceeds the configured maximum
   */
  void process_ctrl_transfer()
  {
    if (m_retry_counter >= m_info.retry_max) {
      m_retry_counter = 0;
      throw hal::io_error(this);
    }

    if (not m_event) {
      return;
    }

    auto const event = m_event.value();

    using bus_event = v5::usb::bus_event;
    auto const previous_retry_count = m_retry_counter;

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

    // If the retry count hasn't increased, then communication was successful
    // and we can reset the retry counter.
    if (previous_retry_count == m_retry_counter) {
      m_retry_counter = 0;
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
      // TODO(#99): Add code to limit the amount of data transmitted and return
      // the amount actually written.
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

  struct configuration_totals
  {
    u16 length;
    u8 interface_count;
  };

  /**
   * @brief Prepare interfaces with their starting interface and starting string
   * number.
   *
   * @return configuration_totals - total length and interface count
   */
  configuration_totals prepare_descriptors()
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

    return {
      .length = static_cast<u16>(
        size_counting_endpoint.total_length +
        static_cast<u16>(constants::configuration_descriptor_size)),
      .interface_count = cur_iface_index,
    };
  }

  constexpr auto generate_configuration_descriptor(
    configuration_totals p_totals)
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

    auto const total_length = setup_packet::to_le_u16(p_totals.length);
    descriptor[w_total_length_lo] = total_length[0];
    descriptor[w_total_length_hi] = total_length[1];

    descriptor[b_num_interfaces] = p_totals.interface_count;
    descriptor[b_configuration_value] = 1;
    descriptor[i_configuration] = 0;  // No string index for configuration

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

/**
 * @brief USB Device Enumerator with in-place interface storage.
 *
 * This template class extends the base enumerator to provide storage for a
 * fixed number of interfaces directly within the enumerator object. This
 * eliminates the need to separately allocate and manage interface objects,
 * making it ideal for embedded systems with static or fixed configurations.
 *
 * The template parameter specifies the exact number of interfaces at compile
 * time, ensuring efficient memory usage with no dynamic allocation. Interfaces
 * are passed to the constructor as forwarding references and are stored in an
 * internal array.
 *
 * @tparam InterfaceCount The number of interfaces this enumerator will manage.
 *
 * Example usage:
 *
 * @code
 * auto usb_cdc = make_strong<usb::cdc_acm_interface>(...);
 * auto usb_hid = make_strong<usb::hid_interface>(...);
 * inplace_enumerator<2> enumerator(ctrl_ep, device_info, usb_cdc, usb_hid);
 * @endcode
 */
template<usize InterfaceCount>
class inplace_enumerator : public enumerator
{
public:
  /**
   * @brief Construct an inplace enumerator with interfaces.
   *
   * Initializes the enumerator with a control endpoint, device configuration,
   * and a variable number of interface objects. All interfaces are forwarded
   * directly into the enumerator's internal storage array, avoiding the need
   * for external allocation or management.
   *
   * All interface objects must be derived from hal::usb::interface.
   *
   * @tparam Interfaces Parameter pack of interface types, all derived from
   *                    hal::usb::interface
   * @param p_ctrl_ep Strong pointer to the device's control endpoint
   * @param p_info Configuration information for the USB device
   * @param p_interfaces Variable number of interface objects to manage
   */
  template<typename... Interfaces>
    requires(std::derived_from<Interfaces, hal::usb::interface> && ...)
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
  std::array<strong_ptr<hal::usb::interface>, InterfaceCount>
    m_interface_storage;
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
