#pragma once

#include <array>
#include <cstddef>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>
#include <optional>
#include <string_view>

#include "descriptors.hpp"
#include "utils.hpp"

// TODO: move to util
namespace hal::v5::usb {

template<typename T>
size_t scatter_span_size(scatter_span<T> ss)
{
  size_t res = 0;
  for (auto const& s : ss) {
    res += s.size();
  }

  return res;
}

template<size_t num_configs>
class enumerator
{

public:
  enumerator(strong_ptr<usb_control_endpoint> const& p_ctrl_ep,
             device&& p_device,
             std::array<configuration, num_configs>&& p_configs,
             std::string_view p_lang_str,
             u8&& p_starting_str_idx)
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
    enumerate();
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
    m_device.manufacturer_index() = cur_str_idx++;
    m_device.product_index() = cur_str_idx++;
    m_device.serial_number_index() = cur_str_idx++;
    m_device.num_configurations() = num_configs;

    // Configurations
    for (auto i = 0; i < num_configs; i++) {
      configuration& config = m_configs[i];
      config.configuration_index() = cur_str_idx++;
      config.configuration_value() = i;
    }

    for (configuration& config : m_configs) {
      auto total_length = static_cast<u16>(constants::config_desc_size);
      for (auto const& iface : config.interfaces) {
        auto deltas = iface->write_descriptors(
          [&total_length](scatter_span<hal::byte const> p_data) {
            total_length += scatter_span_size(p_data);
          });

        cur_iface_idx += deltas.iface_idxes;
        cur_str_idx += deltas.str_idxes;
      }
      config.total_length() = total_length;
    }

    // Phase two: Writing

    // TODO: Make async
    bool finished_enumeration = false;
    bool volatile waiting_for_data = true;

    using on_receive_tag = usb_control_endpoint::on_receive_tag;
    using standard_request_types = setup_request::standard_request_types;
    m_ctrl_ep->on_receive(
      [&waiting_for_data](on_receive_tag) { waiting_for_data = false; });
    m_ctrl_ep->connect(true);
    std::array<hal::byte, constants::size_std_req> raw_req;
    do {
      // Seriously, make this async
      while (waiting_for_data) {
        continue;
      }
      waiting_for_data = true;

      auto scatter_raw_req = make_writable_scatter_bytes(raw_req);
      auto num_bytes_read = m_ctrl_ep->read(scatter_raw_req);

      if (num_bytes_read != constants::size_std_req) {
        safe_throw(hal::message_size(this));
      }
      setup_request req(raw_req);

      if (req.request_type.get_recipient() !=
          setup_request::bitmap::recipient::device) {
        safe_throw(hal::not_connected(this));
      }

      // TODO: Handle exception
      handle_standard_device_request(req);
      m_ctrl_ep->write({});  // Send ZLP to complete Data Transaction
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
    using std_req_type = setup_request::standard_request_types;
    using req_bitmap = setup_request::bitmap;

    while (!m_has_setup_packet) {
      continue;
    }

    std::array<byte, 8> read_buf;
    auto scatter_read_buf = make_writable_scatter_bytes(read_buf);
    auto bytes_read = m_ctrl_ep->read(scatter_read_buf);
    std::span payload(read_buf.data(), bytes_read);

    setup_request req(payload);
    if (req.get_standard_request() == std_req_type::invalid) {
      return;
    }

    if (determine_standard_request(req) ==
          standard_request_types::get_descriptor &&
        static_cast<descriptor_type>((req.value & 0xFF << 8) >> 8) ==
          descriptor_type::string) {
      handle_str_descriptors(req.value & 0xFF, req.length > 2);

    } else if (req.request_type.get_recipient() ==
               req_bitmap::recipient::device) {
      handle_standard_device_request(req);
    } else {
      // Handle iface level requests
      auto f = [this](scatter_span<hal::byte const> resp) {
        m_ctrl_ep->write(resp);
      };
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
  void handle_standard_device_request(setup_request& req)
  {
    using standard_request_types = setup_request::standard_request_types;

    switch (req.get_standard_request()) {
      case standard_request_types::set_address: {
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
        m_active_conf = &m_configs[req.value];
        break;
      }

      case standard_request_types::invalid:
      default:
        safe_throw(hal::not_connected(this));
    }
  }

  void process_get_descriptor(setup_request& req)
  {
    hal::byte desc_type = (req.value & 0xFF << 8) >> 8;
    [[maybe_unused]] hal::byte desc_idx = req.value & 0xFF;

    switch (static_cast<descriptor_type>(desc_type)) {
      case descriptor_type::device: {
        auto header =
          std::to_array({ constants::device_desc_size,
                          static_cast<byte const>(descriptor_type::device) });
        m_device.max_packet_size() =
          static_cast<byte const>(m_ctrl_ep->info().size);
        auto scatter_arr = make_scatter_bytes(header, m_device);
        m_ctrl_ep->write(static_cast<scatter_span<byte const>>(scatter_arr));
        break;
      }

      case descriptor_type::configuration: {
        configuration& conf = m_configs[desc_idx];
        if (req.length <= 2) {  // requesting total length
          auto tl = setup_packet::to_le_bytes(conf.total_length());
          auto scatter_tot_len = make_scatter_bytes(tl);
          m_ctrl_ep->write(scatter_tot_len);
          break;
        }

        // if its >2 then assumed to be requesting desc type
        u16 total_size = constants::config_desc_size;
        auto conf_hdr =
          std::to_array({ constants::config_desc_size,
                          static_cast<byte>(descriptor_type::configuration) });
        auto scatter_conf_hdr =
          make_scatter_bytes(conf_hdr, m_configs[desc_idx]);

        m_ctrl_ep->write(scatter_conf_hdr);

        for (auto const& iface : conf.interfaces) {
          std::ignore = iface->write_descriptors(
            { .interface = std::nullopt, .string = std::nullopt },
            [this, &total_size](scatter_span<hal::byte const> byte_stream) {
              m_ctrl_ep->write(byte_stream);
              total_size += scatter_span_size(byte_stream);
            });
        }

        if (total_size != req.length) {
          safe_throw(
            hal::exception(this));  // TODO: Make specific exception for this
        }

        break;
      }

      case descriptor_type::string: {
        if (desc_idx == 0) {
          auto s_hdr =
            std::to_array({ static_cast<byte const>(m_lang_str.length()),
                            static_cast<byte const>(descriptor_type::string) });
          auto scatter_arr = make_scatter_bytes(
            s_hdr,
            std::span<byte const>(
              reinterpret_cast<byte const*>(m_lang_str.data()),
              m_lang_str.size()));
          m_ctrl_ep->write(scatter_arr);
          break;
        }
        handle_str_descriptors(desc_idx, req.length > 2);  // Can throw
        break;
      }

        // TODO: Interface, endpoint, device_qualifier, interface_power,
        // OTHER_SPEED_CONFIGURATION

      default:
        safe_throw(hal::operation_not_supported(this));
    }
  }

  void handle_str_descriptors(u8 const target_idx, bool const write_full_desc)
  {

    u8 cfg_string_end = m_starting_str_idx + 3 + num_configs;
    if (target_idx <= cfg_string_end) {
      auto r = write_cfg_str_descriptor(target_idx, write_full_desc);
      if (!r) {
        safe_throw(hal::argument_out_of_domain(this));
      }
      m_iface_for_str_desc = std::nullopt;
      return;
    }

    if (m_iface_for_str_desc.has_value() &&
        m_iface_for_str_desc->first == target_idx && write_full_desc) {
      m_iface_for_str_desc->second->write_descriptors(
        [this](scatter_span<hal::byte const> desc) { m_ctrl_ep->write(desc); },
        target_idx);
      return;
    }

    // Iterate through every interface now to find a match
    auto f = [this, write_full_desc](scatter_span<hal::byte const> desc) {
      if (write_full_desc) {
        m_ctrl_ep->write(desc);
      } else {
        std::array<hal::byte const, 1> desc_type{ static_cast<hal::byte const>(
          descriptor_type::string) };
        auto scatter_str_hdr =
          make_scatter_bytes(std::span(&desc[0][0], 1), desc_type);
        m_ctrl_ep->write(scatter_str_hdr);
      }
    };

    if (m_active_conf != nullptr) {
      for (auto const& iface : m_active_conf->interfaces) {
        auto res = iface->write_string_descriptor(f, target_idx);
        if (res) {
          return;
        }
      }
    }

    for (configuration& conf : m_configs) {
      for (auto const& iface : conf.interfaces) {
        auto res = iface->write_string_descriptor(f, target_idx);
        if (res) {
          break;
        }
      }
    }
  }

  bool write_cfg_str_descriptor(u8 const target_idx, bool const write_full_desc)
  {
    constexpr u8 dev_manu_offset = 0;
    constexpr u8 dev_prod_offset = 1;
    constexpr u8 dev_sn_offset = 2;
    std::optional<std::string_view> conf_sv;
    if (target_idx == (m_starting_str_idx + dev_manu_offset)) {
      conf_sv = m_device.manufacturer_str;

    } else if (target_idx == (m_starting_str_idx + dev_prod_offset)) {
      conf_sv = m_device.product_str;

    } else if (target_idx == (m_starting_str_idx + dev_sn_offset)) {
      conf_sv = m_device.serial_number_str;

    } else {
      for (size_t i = 0; i < m_configs.size(); i++) {
        configuration& conf = m_configs[i];
        if (target_idx == (m_starting_str_idx + i)) {
          conf_sv = conf.name;
        }
      }
    }

    if (conf_sv == std::nullopt) {
      return false;
    }

    // Acceptable to access without checking because guaranteed to be Some,
    // there is no pattern matching in C++ yet so unable to do this cleanly
    // (would require a check on every single one)
    auto hdr_arr =
      std::to_array({ static_cast<hal::byte const>(conf_sv->length()),
                      static_cast<hal::byte const>(descriptor_type::string) });
    auto scatter_arr_str_hdr = make_scatter_bytes(hdr_arr);
    m_ctrl_ep->write(scatter_arr_str_hdr);

    if (write_full_desc) {
      // Hack to force scatter_span to accept string view
      auto scatter_arr = make_scatter_bytes(std::span(
        reinterpret_cast<byte const*>(conf_sv->data()), conf_sv->size()));
      m_ctrl_ep->write(scatter_arr);
    }

    return true;
  }

  strong_ptr<usb_control_endpoint> m_ctrl_ep;
  device m_device;
  std::array<configuration, num_configs> m_configs;
  std::string_view m_lang_str;
  u8 m_starting_str_idx;
  std::optional<std::pair<u8, strong_ptr<usb_interface>>> m_iface_for_str_desc;
  configuration* m_active_conf = nullptr;
  bool m_has_setup_packet = false;
};
}  // namespace hal::v5::usb
