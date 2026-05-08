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

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <memory_resource>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

#include <libhal-util/mock/usb.hpp>
#include <libhal-util/usb.hpp>
#include <libhal-util/usb/enumerator.hpp>
#include <libhal-util/usb/utils.hpp>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

#include <boost/ut.hpp>

namespace hal::v5::usb {

boost::ut::suite<"usb_test"> usb_test = [] {
  // TODO(#78): Add usb utility tests
};

struct mock_usb_interface : public hal::usb::interface
{
  descriptor_count driver_write_descriptors(descriptor_start,
                                            endpoint_io&) override
  {
    return {};
  }
  bool driver_write_string_descriptor(u8, endpoint_io&) override
  {
    return false;
  }
  bool driver_handle_request(setup_packet const&, endpoint_io&) override
  {
    return false;
  }
  void driver_handle_host_event(host_event) override
  {
  }
};

boost::ut::suite<"enumeration_test"> enumeration_test = [] {
  using namespace boost::ut;
  using namespace hal::literals;
  namespace pmr = std::pmr;

  std::array<byte, 4096> iface_buf{};
  pmr::monotonic_buffer_resource pool(iface_buf.data(), std::size(iface_buf));

  auto ctrl_ep = make_strong_ptr<mock_usb_control_endpoint>(&pool);
  ctrl_ep->m_endpoint.m_info = { .size = 8, .number = 0, .stalled = false };

  auto usb_interface = make_strong_ptr<mock_usb_interface>(&pool);

  "basic usb enumeration test"_test = [&ctrl_ep, &usb_interface] {
    // Start enumeration process and verify connection
    [[maybe_unused]] inplace_enumerator en(
      ctrl_ep,
      {
        .vendor_id = 0x1209,
        .product_id = 0x0001,
        .manufacturer = u"libhal",
        .product = u"HALbORD",
        .serial_number = u"001",
        .configuration = u"Default",
        // everything else takes its default
      },
      usb_interface);
  };
};

}  // namespace hal::v5::usb
