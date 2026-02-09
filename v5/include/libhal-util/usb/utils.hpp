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

#include <libhal/error.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

namespace hal::v5::usb {

namespace constants {

constexpr byte device_desc_size = 18;
constexpr byte config_desc_size = 9;
constexpr byte inferface_desc_size = 9;
constexpr byte endpoint_desc_size = 7;
constexpr byte iad_desc_size = 0x08;

constexpr byte size_std_req = 8;

}  // namespace constants

// Maybe move these enum classes into the constants namespace
// Assigned by USB-IF
enum class class_code : hal::byte
{
  use_interface_descriptor =
    0x00,               // Use class information in the Interface Descriptors
  audio = 0x01,         // Audio device class
  cdc_control = 0x02,   // Communications and CDC Control
  hid = 0x03,           // Human Interface Device
  physical = 0x05,      // Physical device class
  image = 0x06,         // Still Imaging device
  printer = 0x07,       // Printer device
  mass_storage = 0x08,  // Mass Storage device
  hub = 0x09,           // Hub device
  cdc_data = 0x0A,      // CDC-Data device
  smart_card = 0x0B,    // Smart Card device
  content_security = 0x0D,      // Content Security device
  video = 0x0E,                 // Video device
  personal_healthcare = 0x0F,   // Personal Healthcare device
  audio_video = 0x10,           // Audio/Video Devices
  billboard = 0x11,             // Billboard Device Class
  usb_c_bridge = 0x12,          // USB Type-C Bridge Class
  bulk_display = 0x13,          // USB Bulk Display Protocol Device Class
  mctp = 0x14,                  // MCTP over USB Protocol Endpoint Device Class
  i3c = 0x3C,                   // I3C Device Class
  diagnostic = 0xDC,            // Diagnostic Device
  wireless_controller = 0xE0,   // Wireless Controller
  misc = 0xEF,                  // Miscellaneous
  application_specific = 0xFE,  // Application Specific
  vendor_specific = 0xFF        // Vendor Specific
};

// Default types
enum class descriptor_type : hal::byte
{
  device = 0x1,
  configuration = 0x2,
  string = 0x3,
  interface = 0x4,
  endpoint = 0x5,
  device_qualifier = 0x6,
  other_speed_configuration = 0x7,
  interface_power = 0x8,
  otg = 0x9,
  debug = 0xA,
  interface_association = 0xB,
  security = 0xC,
  key = 0xD,
  encryption_type = 0xE,
  bos = 0xF,
  device_capability = 0x10,
  wireless_endpoint_companion = 0x11,
  superspeed_endpoint_companion = 0x30,
  superspeed_endpoint_isochronous_companion = 0x31,
};

}  // namespace hal::v5::usb

namespace hal::usb {
namespace constants = v5::usb::constants;
using v5::usb::class_code;
using v5::usb::descriptor_type;

}  // namespace hal::usb
