#pragma once

#include <cstddef>
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

/**
 * @brief USB Class Code indicating the type of device or interface. Assigned by
 * USB-IF.
 */
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

/**
 * @brief USB Descriptor Type values as defined by the USB specification.
 * These values are used in the bDescriptorType field of USB descriptors
 * to identify the type and structure of the descriptor data.
 */
enum class descriptor_type : hal::byte
{
  device = 0x1,                        // Device descriptor
  configuration = 0x2,                 // Configuration descriptor
  string = 0x3,                        // String descriptor
  interface = 0x4,                     // Interface descriptor
  endpoint = 0x5,                      // Endpoint descriptor
  device_qualifier = 0x6,              // Device qualifier descriptor
  other_speed_configuration = 0x7,     // Other speed configuration descriptor
  interface_power = 0x8,               // Interface power descriptor
  otg = 0x9,                           // OTG descriptor
  debug = 0xA,                         // Debug descriptor
  interface_association = 0xB,         // Interface Association descriptor
  security = 0xC,                      // Security descriptor
  key = 0xD,                           // Key descriptor
  encryption_type = 0xE,               // Encryption type descriptor
  bos = 0xF,                           // Binary Object Store (BOS) descriptor
  device_capability = 0x10,            // Device capability descriptor
  wireless_endpoint_companion = 0x11,  // Wireless endpoint companion descriptor
  superspeed_endpoint_companion =
    0x30,  // SuperSpeed endpoint companion descriptor
  superspeed_endpoint_isochronous_companion =
    0x31,  // SuperSpeed isochronous endpoint companion descriptor
};

}  // namespace hal::v5::usb
