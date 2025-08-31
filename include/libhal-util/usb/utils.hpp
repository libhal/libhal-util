#pragma once

#include <array>
#include <cstddef>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>
#include <span>

// TODO: Move to util
namespace hal::v5::usb {

// TODO: Make generic
u16 from_le_bytes(hal::byte& first, hal::byte& second)
{
  return static_cast<u16>(second) << 8 | first;
}

std::array<hal::byte, 2> to_le_bytes(u16 n)
{
  return { static_cast<hal::byte>(n & 0xFF),
           static_cast<hal::byte>((n & 0xFF << 8) >> 8) };
}

namespace constants {

constexpr byte device_desc_size = 18;
constexpr byte config_desc_size = 9;
constexpr byte inferface_desc_size = 9;
constexpr byte endpoint_desc_size = 7;
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

struct setup_request
{
  using bitmap = v5::usb_interface::req_bitmap;

  enum class standard_request_types : hal::byte const
  {
    get_status = 0x00,
    clear_feature = 0x01,
    set_feature = 0x03,
    set_address = 0x05,
    get_descriptor = 0x06,
    set_descriptor = 0x07,
    get_configuration = 0x08,
    set_configuration = 0x09,
    get_interface = 0x0A,
    set_interface = 0x11,
    synch_frame = 0x12,
    invalid
  };

  constexpr setup_request(bitmap p_request_type,
                          hal::byte p_request,  // NOLINT
                          u16 p_value,
                          u16 p_index,
                          u16 p_length)
    : request_type(p_request_type)
    , request(p_request)
    , value(p_value)
    , index(p_index)
    , length(p_length) {};

  constexpr setup_request(std::span<byte> raw_req)
    : request_type(raw_req[0])
    , request(raw_req[1])
    , value(from_le_bytes(raw_req[2], raw_req[3]))
    , index(from_le_bytes(raw_req[4], raw_req[5]))
    , length(from_le_bytes(raw_req[6], raw_req[7]))

  {};

  // Probably better semantics for this function, this disjoints
  [[nodiscard]] constexpr standard_request_types get_standard_request() const
  {
    if (request_type.get_type() != bitmap::type::standard || request == 0x04 ||
        request > 0x12) {
      return standard_request_types::invalid;
    }

    return static_cast<standard_request_types>(request);
  }

  bitmap const request_type;
  hal::byte const request;
  u16 const value;
  u16 const index;
  u16 const length;
};

}  // namespace hal::v5::usb
