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

#include <array>
#include <libhal/allocated_buffer.hpp>
#include <memory_resource>
#include <span>
#include <string_view>
#include <vector>

#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

#include "utils.hpp"

/* TODO (PhazonicRidley):
  Class, subclass, proto validator
  Device qualifer descriptor (happens between device and config)
  Other speed descriptor (happens with configuration)
*/

namespace hal::v5::usb {

struct device
{
  template<size_t>
  friend class enumerator;

  struct device_arguments
  {
    u16 bcd_usb;
    class_code device_class;
    u8 device_subclass;
    u8 device_protocol;
    u16 id_vendor;
    u16 id_product;
    u16 bcd_device;
    std::u16string_view p_manufacturer;
    std::u16string_view p_product;
    std::u16string_view p_serial_number_str;
  };

  constexpr device(device_arguments&& p_args)
    : manufacturer_str(p_args.p_manufacturer)
    , product_str(p_args.p_product)
    , serial_number_str(p_args.p_serial_number_str)
  {
    u8 idx = 0;
    auto bcd_usb_bytes = setup_packet::to_le_u16(p_args.bcd_usb);
    for (auto& bcd_usb_byte : bcd_usb_bytes) {
      m_packed_arr[idx++] = bcd_usb_byte;
    }
    m_packed_arr[idx++] = static_cast<u8>(p_args.device_class);
    m_packed_arr[idx++] = p_args.device_subclass;
    m_packed_arr[idx++] = p_args.device_protocol;

    m_packed_arr[idx++] = 0;  // Max Packet length handled by the enumerator
    auto id_vendor_bytes = setup_packet::to_le_u16(p_args.id_vendor);
    for (auto& id_vendor_byte : id_vendor_bytes) {
      m_packed_arr[idx++] = id_vendor_byte;
    }

    auto id_product_bytes = setup_packet::to_le_u16(p_args.id_product);
    for (auto& id_product_byte : id_product_bytes) {
      m_packed_arr[idx++] = id_product_byte;
    }

    auto bcd_device_bytes = setup_packet::to_le_u16(p_args.bcd_device);
    for (auto& bcd_device_byte : bcd_device_bytes) {
      m_packed_arr[idx++] = bcd_device_byte;
    }

    // Default string indexes, assuming enumeration will use 4 onward for string
    // indexes are these are required (can be modified by enumerator if desired)
    m_packed_arr[idx++] = 1;  // string idx of manufacturer
    m_packed_arr[idx++] = 2;  // string idx of product
    m_packed_arr[idx++] = 3;  // string idx of serial number

    // Evaluated during enumeration
    m_packed_arr[idx++] = 0;  // Number of possible configurations
  };

  [[nodiscard]] constexpr u16 bcd_usb() const
  {
    return setup_packet::from_le_bytes(m_packed_arr[0], m_packed_arr[1]);
  }

  [[nodiscard]] constexpr u8 device_class() const
  {
    return m_packed_arr[2];
  }

  [[nodiscard]] constexpr u8 device_sub_class() const
  {
    return m_packed_arr[3];
  }

  [[nodiscard]] constexpr u8 device_protocol() const
  {
    return m_packed_arr[4];
  }

  [[nodiscard]] constexpr u16 id_vendor() const
  {
    return setup_packet::from_le_bytes(m_packed_arr[6], m_packed_arr[7]);
  }

  [[nodiscard]] constexpr u16 id_product() const
  {
    return setup_packet::from_le_bytes(m_packed_arr[8], m_packed_arr[9]);
  }

  [[nodiscard]] constexpr u16 bcd_device() const
  {
    return setup_packet::from_le_bytes(m_packed_arr[10], m_packed_arr[11]);
  }

  operator std::span<u8 const>() const
  {
    return m_packed_arr;
  }

  std::u16string_view manufacturer_str;
  std::u16string_view product_str;
  std::u16string_view serial_number_str;

private:
  [[nodiscard]] constexpr u8 max_packet_size() const
  {
    return m_packed_arr[5];
  }
  constexpr void set_max_packet_size(u8 p_size)
  {
    m_packed_arr[5] = p_size;
  }

  [[nodiscard]] constexpr u8 manufacturer_index() const
  {
    return m_packed_arr[12];
  }

  [[nodiscard]] constexpr u8 product_index() const
  {
    return m_packed_arr[13];
  }

  [[nodiscard]] constexpr u8 serial_number_index() const
  {
    return m_packed_arr[14];
  }

  [[nodiscard]] constexpr u8 num_configurations() const
  {
    return m_packed_arr[15];
  }
  constexpr void set_num_configurations(u8 p_count)
  {
    m_packed_arr[15] = p_count;
  }

  std::array<hal::byte, 16> m_packed_arr;
};

// https://www.beyondlogic.org/usbnutshell/usb5.shtml#ConfigurationDescriptors

template<typename T>
concept usb_interface_concept = std::derived_from<T, interface>;

// Calculate: total length, number of interfaces, configuration value
struct configuration
{

  template<size_t>
  friend class enumerator;

  struct bitmap
  {

    constexpr bitmap(u8 p_bitmap)
      : m_bitmap(p_bitmap)
    {
    }

    constexpr bitmap(bool p_self_powered, bool p_remote_wakeup)
    {
      m_bitmap = (1 << 7) | (p_self_powered << 6) | (p_remote_wakeup << 5);
    }

    constexpr u8 to_byte()
    {
      return m_bitmap;
    }

    [[nodiscard]] constexpr bool self_powered() const
    {
      return static_cast<bool>(m_bitmap & 1 << 6);
    }

    [[nodiscard]] constexpr bool remote_wakeup() const
    {
      return static_cast<bool>(m_bitmap & 1 << 5);
    }

  private:
    u8 m_bitmap;
  };

  /** Configuration descriptor info.
   * @note The name string_view must outlive the configuration object.
   */
  struct configuration_info
  {
    std::u16string_view name;
    bitmap attributes;
    u8 max_power;
    std::pmr::polymorphic_allocator<> allocator;
  };

  template<usb_interface_concept... Interfaces>
  constexpr configuration(configuration_info p_info,
                          strong_ptr<Interfaces>... p_interfaces)
    : name(p_info.name)
    , interfaces(p_info.allocator)
  {
    interfaces.reserve(sizeof...(p_interfaces));
    (interfaces.push_back(p_interfaces), ...);
    u8 idx = 0;

    // Anything marked with 0 is to be populated at enumeration time
    m_packed_arr[idx++] = 0;  // 0 Total Length
    m_packed_arr[idx++] = 0;
    m_packed_arr[idx++] = interfaces.size();  // 2 number of interfaces
    m_packed_arr[idx++] = 0;                  // 3 Config number
    m_packed_arr[idx++] = 0;  // 4 Configuration name string index

    m_packed_arr[idx++] = p_info.attributes.to_byte();  // 5
    m_packed_arr[idx++] = p_info.max_power;             // 6
  }

  operator std::span<u8 const>() const
  {
    return m_packed_arr;
  }

  constexpr bitmap attributes()
  {
    return { m_packed_arr[5] };
  }
  [[nodiscard]] constexpr u8 attributes_byte() const
  {
    return m_packed_arr[5];
  }

  [[nodiscard]] constexpr u8 max_power() const
  {
    return m_packed_arr[6];
  }

  std::u16string_view name;
  std::pmr::vector<strong_ptr<interface>> interfaces;

private:
  [[nodiscard]] constexpr u16 total_length() const
  {
    return setup_packet::from_le_bytes(m_packed_arr[0], m_packed_arr[1]);
  }

  constexpr void set_total_length(u16 p_length)
  {
    auto bytes = setup_packet::to_le_u16(p_length);
    m_packed_arr[0] = bytes[0];
    m_packed_arr[1] = bytes[1];
  }
  [[nodiscard]] constexpr u8 num_interfaces() const
  {
    return m_packed_arr[2];
  }
  constexpr void set_num_interfaces(u8 p_count)
  {
    m_packed_arr[2] = p_count;
  }

  [[nodiscard]] constexpr u8 configuration_value() const
  {
    return m_packed_arr[3];
  }
  constexpr void set_configuration_value(u8 p_value)
  {
    m_packed_arr[3] = p_value;
  }

  [[nodiscard]] constexpr u8 configuration_index() const
  {
    return m_packed_arr[4];
  }
  constexpr void set_configuration_index(u8 p_index)
  {
    m_packed_arr[4] = p_index;
  }

  std::array<hal::byte, 7> m_packed_arr;
};
}  // namespace hal::v5::usb

namespace hal::usb {
using v5::usb::configuration;
using v5::usb::device;
using v5::usb::usb_interface_concept;
}  // namespace hal::usb
