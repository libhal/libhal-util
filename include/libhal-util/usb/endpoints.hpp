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
#include <libhal/scatter_span.hpp>
#include <libhal/units.hpp>
#include <libhal/usb.hpp>

namespace hal::v5::usb {
// TODO(#79): Add doxygen docs to USB APIs
inline void write(control_endpoint& p_endpoint,
                  scatter_span<hal::byte const> p_data_out)
{
  p_endpoint.write(p_data_out);
}

inline void write_and_flush(control_endpoint& p_endpoint,
                            scatter_span<hal::byte const> p_data_out)
{
  p_endpoint.write(p_data_out);
  p_endpoint.write({});
}

inline void write(control_endpoint& p_endpoint,
                  std::span<hal::byte const> p_data_out)
{
  p_endpoint.write(make_scatter_bytes(p_data_out));
}

inline void write_and_flush(control_endpoint& p_endpoint,
                            std::span<hal::byte const> p_data_out)
{
  p_endpoint.write(make_scatter_bytes(p_data_out));
  p_endpoint.write({});
}

inline void write(in_endpoint& p_endpoint,
                  scatter_span<hal::byte const> p_data_out)
{
  p_endpoint.write(p_data_out);
}

inline void write_and_flush(in_endpoint& p_endpoint,
                            scatter_span<hal::byte const> p_data_out)
{
  p_endpoint.write(p_data_out);
  p_endpoint.write({});
}

inline void write(in_endpoint& p_endpoint,
                  std::span<hal::byte const> p_data_out)
{
  p_endpoint.write(make_scatter_bytes(p_data_out));
}

inline void write_and_flush(in_endpoint& p_endpoint,
                            std::span<hal::byte const> p_data_out)
{
  p_endpoint.write(make_scatter_bytes(p_data_out));
  p_endpoint.write({});
}

inline void write(in_endpoint& p_endpoint, spanable_bytes auto... p_data_out)
{
  p_endpoint.write(make_scatter_bytes(p_data_out...));
}

inline void write_and_flush(in_endpoint& p_endpoint,
                            spanable_bytes auto... p_data_out)
{
  p_endpoint.write(make_scatter_bytes(p_data_out...));
  p_endpoint.write({});
}

inline auto read(out_endpoint& p_endpoint, scatter_span<hal::byte> p_data_out)
{
  return p_endpoint.read(p_data_out);
}

inline auto read(out_endpoint& p_endpoint, std::span<hal::byte> p_data_out)
{
  return p_endpoint.read(make_writable_scatter_bytes(p_data_out));
}

inline auto read(out_endpoint& p_endpoint,
                 spanable_writable_bytes auto... p_data_out)
{
  return p_endpoint.read(make_writable_scatter_bytes(p_data_out...));
}

inline auto read(control_endpoint& p_endpoint,
                 scatter_span<hal::byte> p_data_out)
{
  return p_endpoint.read(p_data_out);
}

inline auto read(control_endpoint& p_endpoint, std::span<hal::byte> p_data_out)
{
  return p_endpoint.read(make_writable_scatter_bytes(p_data_out));
}

inline auto read(control_endpoint& p_endpoint,
                 spanable_writable_bytes auto... p_data_out)
{
  return p_endpoint.read(make_writable_scatter_bytes(p_data_out...));
}
}  // namespace hal::v5::usb
