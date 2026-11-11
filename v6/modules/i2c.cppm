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

module;

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <mutex>
#include <type_traits>

export module hal.util:i2c;

import hal;
import scatter_span;

import :enumeration;

/**
 * @defgroup I2CUtils I2C Utils
 */
namespace hal::inline v6 {
/**
 * @ingroup I2CUtils
 * @brief write data to a target device on the i2c bus
 *
 * Shorthand for writing i2c.transaction(...) for write only operations.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 */
export void write(hal::i2c& p_i2c,
                  hal::byte p_address,
                  mem::scatter_span<hal::byte const> p_data_out)
{
  p_i2c.transaction(p_address, p_data_out, {});
}

/**
 * @ingroup I2CUtils
 * @brief read bytes from target device on i2c bus
 *
 * Shorthand for writing i2c.transaction(...) for read only operations.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_in - buffer to read bytes into from target device
 */
export void read(hal::i2c& p_i2c,
                 hal::byte p_address,
                 mem::scatter_span<hal::byte> p_data_in)
{
  p_i2c.transaction(p_address, {}, p_data_in);
}

/**
 * @ingroup I2CUtils
 * @brief return array of read bytes from target device on i2c bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @return std::array<hal::byte, bytes_to_read> - array of bytes from target
 * device
 */
export template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> read(hal::i2c& p_i2c,
                                                        hal::byte p_address)
{
  std::array<hal::byte, bytes_to_read> buffer;
  read(p_i2c, p_address, { buffer });
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief write and then read bytes from target device on i2c bus
 *
 * This API simply calls transaction. This API is here for consistency across
 * the other other communication protocols such as SPI and serial.
 *
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @param p_data_in - buffer to read bytes into from target device
 */
export void write_then_read(hal::i2c& p_i2c,
                            hal::byte p_address,
                            mem::scatter_span<hal::byte const> p_data_out,
                            mem::scatter_span<hal::byte> p_data_in)
{
  p_i2c.transaction(p_address, p_data_out, p_data_in);
}

/**
 * @ingroup I2CUtils
 * @brief write and then return an array of read bytes from target device on i2c
 * bus
 *
 * Eliminates the need to create a buffer and pass it into the read function.
 *
 * @tparam bytes_to_read - number of bytes to read after write
 * @param p_i2c - i2c driver
 * @param p_address - target address
 * @param p_data_out - buffer of bytes to write to the target device
 * @return std::array<hal::byte, bytes_to_read> -
 */
export template<size_t bytes_to_read>
[[nodiscard]] std::array<hal::byte, bytes_to_read> write_then_read(
  hal::i2c& p_i2c,
  hal::byte p_address,
  mem::scatter_span<hal::byte const> p_data_out)
{
  std::array<hal::byte, bytes_to_read> buffer;
  write_then_read(p_i2c, p_address, p_data_out, { buffer });
  return buffer;
}

/**
 * @ingroup I2CUtils
 * @brief probe the i2c bus to see if a device exists
 *
 * NOTE: that this utilizes the fact that i2c drivers throw
 * hal::no_such_device when a transaction is performed and the device's
 * address is used on the bus and the device does not respond with an
 * acknowledge.
 *
 * @param p_i2c - i2c driver for the i2c bus that the device may exist on
 * @param p_address - device to check
 * @return true - if the device appears on the bus
 * @return false - if the devices does not appear on the i2c bus
 */
export [[nodiscard]] bool probe(hal::i2c& p_i2c, hal::byte p_address)
{
  // p_data_in: empty placeholder for transaction's data_in
  std::array<hal::byte, 1> data_in{};
  bool device_acknowledged = true;

  try {
    p_i2c.transaction(p_address, {}, { data_in });
  } catch (hal::no_such_device const&) {
    device_acknowledged = false;
  }

  return device_acknowledged;
}

/**
 * @ingroup I2CUtils
 * @brief Set of I2C transaction operations
 *
 */
export enum class i2c_operation : u8 {
  /// Denotes an i2c write operation
  write = 0,
  /// Denotes an i2c read operation
  read = 1,
};

/**
 * @ingroup I2CUtils
 * @brief Convert 7-bit i2c address to an 8-bit address
 *
 * @param p_address 7-bit i2c address
 * @param p_operation write or read operation
 * @return hal::byte - 8-bit i2c address
 */
export [[nodiscard]] inline hal::byte to_8_bit_address(
  hal::byte p_address,
  i2c_operation p_operation) noexcept
{
  auto v8bit_address = static_cast<hal::byte>(p_address << 1);
  v8bit_address |= hal::value(p_operation);
  return v8bit_address;
}

/**
 * @ingroup I2CUtils
 * @brief The part of every `hal::thread_safe_i2c` that doesn't depend on its
 * bus type
 *
 * Holds the lock around each transaction it forwards to the bus.
 */
class thread_safe_i2c_base : public hal::i2c
{
protected:
  /**
   * @param p_bus - the bus transactions are forwarded to
   * @param p_lock - held around each transaction
   */
  thread_safe_i2c_base(hal::i2c& p_bus, hal::basic_lock& p_lock)
    : m_bus(p_bus)
    , m_lock(p_lock)
  {
  }

  ~thread_safe_i2c_base() = default;

private:
  void driver_transaction(hal::byte p_address,
                          mem::scatter_span<hal::byte const> p_data_out,
                          mem::scatter_span<hal::byte> p_data_in) override
  {
    // Not std::scoped_lock, which libc++ built without threads doesn't provide.
    std::lock_guard const hold(*m_lock);
    m_bus->transaction(p_address, p_data_out, p_data_in);
  }

  hal::ref<hal::i2c> m_bus;
  hal::ref<hal::basic_lock> m_lock;
};

/// Holds the bus of a `hal::thread_safe_i2c` that owns it. It is a separate
/// base, listed first, so the bus is fully constructed before
/// `thread_safe_i2c_base` refers to it.
template<class Bus>
class thread_safe_i2c_storage
{
protected:
  template<class Factory>
  explicit thread_safe_i2c_storage(Factory& p_factory)
    : m_bus(p_factory())
  {
  }

  Bus m_bus;
};

/// A `hal::thread_safe_i2c` that refers to a bus owned elsewhere stores nothing
/// extra.
template<>
class thread_safe_i2c_storage<hal::i2c>
{};

/**
 * @ingroup I2CUtils
 * @brief Makes an i2c bus safe to use from more than one thread or context
 *
 * Holds a `hal::basic_lock` for the whole of each `transaction()`. A
 * transaction started while another is still running waits for it to finish
 * instead of failing with `hal::device_or_resource_busy`. It is a `hal::i2c`
 * itself, so device drivers take it in place of the bus. An owned bus gets
 * its settings from the factory that builds it.
 *
 * Drivers that all run in one context don't need it and can share the bus
 * directly.
 *
 * It is only as safe as its lock. The lock must block, such as an RTOS
 * mutex. If an interrupt uses the bus, the lock must also be safe to take
 * from that interrupt, which most RTOS mutexes aren't.
 *
 * It either owns its bus or refers to one that lives elsewhere. Owning it is
 * safer: built from a factory, the bus exists only inside it, so no context
 * can use the bus without taking the lock.
 *
 * ```cpp
 * // Owns the bus. The factory's return type is the bus type.
 * static hal::thread_safe_i2c i2c1(
 *   [] { return hal::stm32f1::i2c(hal::stm32f1::peripheral::i2c1, clock); },
 *   rtos_mutex);
 *
 * // Refers to a bus that lives elsewhere. Every user must go through i2c2.
 * static hal::stm32f1::i2c bus2(hal::stm32f1::peripheral::i2c2, clock);
 * static hal::thread_safe_i2c i2c2(bus2, rtos_mutex);
 *
 * static hal::sensors::mpu6050 imu(i2c1);
 * ```
 *
 * The lock and a bus it doesn't own must outlive it.
 *
 * @tparam Bus - the bus type it owns, or `hal::i2c` (the default) to refer to
 * a bus that lives elsewhere
 */
export template<class Bus = hal::i2c>
class thread_safe_i2c final
  : private thread_safe_i2c_storage<Bus>
  , public thread_safe_i2c_base
{
public:
  static_assert(std::derived_from<Bus, hal::i2c>,
                "The bus type must implement hal::i2c");

  /**
   * @brief Lock a bus that lives elsewhere
   *
   * @param p_bus - the bus to share between contexts
   * @param p_lock - held around each transaction
   */
  thread_safe_i2c(hal::ref<hal::i2c> p_bus [[clang::lifetimebound]],
                  hal::ref<hal::basic_lock> p_lock [[clang::lifetimebound]])
    requires std::same_as<Bus, hal::i2c>
    : thread_safe_i2c_base(*p_bus, *p_lock)
  {
  }

  /**
   * @brief Build the bus inside and lock it
   *
   * @param p_factory - called once to build the bus in place. The bus can be
   * non-movable. Anything it captures by reference must outlive this object.
   * @param p_lock - held around each transaction
   */
  template<class Factory>
    requires(not std::same_as<Bus, hal::i2c> and
             std::same_as<std::invoke_result_t<Factory&>, Bus>)
  thread_safe_i2c(Factory p_factory [[clang::lifetimebound]],
                  hal::ref<hal::basic_lock> p_lock [[clang::lifetimebound]])
    : thread_safe_i2c_storage<Bus>(p_factory)
    , thread_safe_i2c_base(thread_safe_i2c_storage<Bus>::m_bus, *p_lock)
  {
  }

  thread_safe_i2c(thread_safe_i2c const&) = delete;
  thread_safe_i2c& operator=(thread_safe_i2c const&) = delete;
  thread_safe_i2c(thread_safe_i2c&&) = delete;
  thread_safe_i2c& operator=(thread_safe_i2c&&) = delete;
  ~thread_safe_i2c() = default;
};

/// A bus object and a lock: it refers to the bus.
export thread_safe_i2c(hal::i2c&, hal::basic_lock&)
  -> thread_safe_i2c<hal::i2c>;

/// A factory and a lock: it owns the bus the factory returns.
export template<class Factory>
  requires std::invocable<Factory&>
thread_safe_i2c(Factory, hal::basic_lock&)
  -> thread_safe_i2c<std::invoke_result_t<Factory&>>;

/**
 * @ingroup I2CUtils
 * @brief A device driver type that declares the fastest i2c clock rate its
 * device supports
 *
 * Satisfied by a type with a `static constexpr hal::hertz max_clock_rate`.
 */
export template<class T>
concept i2c_rated_device = requires {
  { T::max_clock_rate } -> std::convertible_to<hal::hertz>;
};

/**
 * @ingroup I2CUtils
 * @brief The fastest clock rate an i2c bus can run at, computed at compile
 * time from the devices on it
 *
 * Every device on a bus sees its clock, so the bus can run no faster than its
 * slowest device. List every device wired to the bus, whether or not the
 * program uses it: the compiler can't check that the list matches the board.
 *
 * ```cpp
 * static hal::stm32f1::i2c i2c1(hal::stm32f1::peripheral::i2c1, clock, {
 *   .clock_rate = hal::i2c_bus_rate<mpu6050, at24c02>(400 * kHz),
 * });
 * ```
 *
 * @tparam Devices - the driver type of each device on the bus. A type that
 * doesn't declare `max_clock_rate` fails to compile.
 * @param p_board_limit - the fastest rate the board's wiring supports, which
 * depends on its pull-up resistors and the bus's capacitance
 * @return hal::hertz - the lowest of `p_board_limit` and every device's
 * `max_clock_rate`
 */
export template<i2c_rated_device... Devices>
[[nodiscard]] consteval hal::hertz i2c_bus_rate(hal::hertz p_board_limit)
{
  return std::min(
    { p_board_limit, static_cast<hal::hertz>(Devices::max_clock_rate)... });
}

/**
 * @ingroup I2CUtils
 * @brief A device driver type that declares how long its device stretches the
 * i2c clock
 *
 * Satisfied by a type with a `static constexpr hal::time_duration
 * max_clock_stretch`: the longest the device holds SCL low during one
 * transaction, beyond a little on each byte. A device that doesn't stretch the
 * clock declares 0.
 */
export template<class T>
concept i2c_stretching_device = requires {
  { T::max_clock_stretch } -> std::convertible_to<hal::time_duration>;
};

/**
 * @ingroup I2CUtils
 * @brief The slack an i2c bus needs, computed at compile time from the devices
 * on it
 *
 * The slack, `hal::i2c_settings::slack`, is how much longer than twice its own
 * length a transaction may take: see "Timeout" in `hal::i2c`. A transaction
 * talks to one device, so the bus needs room for the device that stretches the
 * clock the longest. The application adds its own allowance for how long a
 * transaction can wait on the calling context, for example while an interrupt
 * or a higher priority thread runs. List every device wired to the bus, as for
 * `hal::i2c_bus_rate()`.
 *
 * ```cpp
 * static hal::stm32f1::polled_i2c i2c1(hal::stm32f1::peripheral::i2c1, clock, {
 *   .clock_rate = hal::i2c_bus_rate<sht21, at24c02>(400 * kHz),
 *   .slack = hal::i2c_bus_slack<sht21, at24c02>(),
 * });
 * ```
 *
 * @tparam Devices - the driver type of each device on the bus. A type that
 * doesn't declare `max_clock_stretch` fails to compile.
 * @param p_allowance - how long a transaction may wait on the calling context.
 * The default, 1 ms, is the default slack, and suits a context that isn't
 * interrupted for long.
 * @return hal::time_duration - `p_allowance` plus the longest
 * `max_clock_stretch` of the devices
 */
export template<i2c_stretching_device... Devices>
[[nodiscard]] consteval hal::time_duration i2c_bus_slack(
  hal::time_duration p_allowance = std::chrono::milliseconds(1))
{
  return p_allowance + std::max({ hal::time_duration::zero(),
                                  static_cast<hal::time_duration>(
                                    Devices::max_clock_stretch)... });
}

/**
 * @ingroup I2CUtils
 * @brief The clock cycles an i2c transaction takes on the bus, without
 * stretching
 *
 * This is a transaction's own length, as "Timeout" in `hal::i2c` counts it: 9
 * cycles for every byte, address bytes included, plus one for each start and
 * stop condition. A write-then-read sends the address twice and has a repeated
 * start. An implementation gives a transaction twice this many cycles at its
 * clock rate, plus the slack, before throwing `hal::timed_out`.
 *
 * @tparam Count - the unsigned type to count in. The default, `hal::u64`, holds
 * the count for any transaction. `hal::u32` takes fewer instructions on a
 * 32-bit processor and holds the count for transactions of up to 477 million
 * bytes. The count isn't checked for overflow, so pick a type wide enough for
 * the longest transaction.
 * @param p_bytes_out - the number of bytes written
 * @param p_bytes_in - the number of bytes read
 * @return Count - the clock cycles, or 0 for a transaction with nothing to
 * write or read, which doesn't touch the bus
 */
export template<std::unsigned_integral Count = hal::u64>
[[nodiscard]] constexpr Count i2c_transaction_clocks(hal::usize p_bytes_out,
                                                     hal::usize p_bytes_in)
{
  constexpr Count clocks_per_byte = 9;
  // Each direction has an address byte and a start condition.
  Count const directions = Count{ p_bytes_out != 0 } + Count{ p_bytes_in != 0 };
  if (directions == 0) {
    return 0;
  }
  Count const bytes =
    (static_cast<Count>(p_bytes_out) + static_cast<Count>(p_bytes_in)) +
    directions;
  // The start conditions, plus the stop condition
  return (bytes * clocks_per_byte) + (directions + 1);
}
}  // namespace hal::inline v6
