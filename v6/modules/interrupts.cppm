// Copyright 2026 Khalil Estell and the libhal contributors
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

#include <array>
#include <cstddef>

export module hal.util:interrupts;

import hal;

/**
 * @defgroup Interrupt_Utilities Interrupt Utilities
 *
 */

namespace hal::inline v6 {
export template<std::size_t SlotCount>
class edge_triggered_interrupt_dispatcher;

/**
 * @brief One subscriber's view of a pin shared by
 * `hal::edge_triggered_interrupt_dispatcher`
 * @ingroup Interrupt_Utilities
 *
 * Only a dispatcher can create slots; get them from its `operator[]`. Every
 * dispatcher, whatever its slot count, uses this one type.
 *
 * A slot starts with the trigger setting `edge_trigger::rising`. Configuring
 * a slot changes only which edges reach its subscriber, not the real pin.
 */
export class edge_triggered_interrupt_slot final
  : public edge_triggered_interrupt
{
public:
  edge_triggered_interrupt_slot(edge_triggered_interrupt_slot const&) = delete;
  edge_triggered_interrupt_slot& operator=(
    edge_triggered_interrupt_slot const&) = delete;
  edge_triggered_interrupt_slot(edge_triggered_interrupt_slot&&) = delete;
  edge_triggered_interrupt_slot& operator=(edge_triggered_interrupt_slot&&) =
    delete;
  ~edge_triggered_interrupt_slot() = default;

private:
  template<std::size_t SlotCount>
  friend class edge_triggered_interrupt_dispatcher;

  edge_triggered_interrupt_slot() = default;

  void dispatch(bool p_state) noexcept
  {
    // Read the callback once: a subscriber can be removed between edges.
    auto* const callback = m_callback;
    if (callback == nullptr) {
      return;
    }
    if (m_trigger == edge_trigger::both ||
        (m_trigger == edge_trigger::rising && p_state) ||
        (m_trigger == edge_trigger::falling && not p_state)) {
      callback->callback(p_state);
    }
  }

  void driver_configure(settings const& p_settings) override
  {
    m_trigger = p_settings.trigger;
  }

  void driver_on_trigger(edge_triggered_callback& p_callback) override
  {
    if (m_callback != nullptr) {
      throw subscription_busy(this);
    }
    m_callback = &p_callback;
  }

  void driver_clear_trigger() noexcept override
  {
    m_callback = nullptr;
  }

  edge_triggered_callback* m_callback = nullptr;
  edge_trigger m_trigger = edge_trigger::rising;
};

/**
 * @brief Shares one edge triggered interrupt among several subscribers
 * @ingroup Interrupt_Utilities
 *
 * `hal::edge_triggered_interrupt` accepts one subscriber at a time. The
 * dispatcher subscribes once to a real pin and provides `SlotCount` slots.
 * Each slot is a `hal::edge_triggered_interrupt_slot`, a
 * `hal::edge_triggered_interrupt` with its own subscriber and its own trigger
 * setting.
 *
 * The dispatcher configures the real pin to trigger on both edges. When an
 * edge occurs, it calls, in slot order, each slot's subscriber whose trigger
 * setting matches the new pin state. That happens in interrupt context, and
 * costs O(SlotCount) per edge.
 *
 * Neither the dispatcher nor its slots may outlive the pin, and the slots
 * may not outlive the dispatcher.
 *
 * Usage:
 *
 * ```cpp
 * hal::edge_triggered_interrupt_dispatcher<2> dispatcher(pin);
 * button_counter counter(dispatcher[0]);
 * wake_up_handler wake_up(dispatcher[1]);
 * ```
 *
 * @tparam SlotCount - number of slots
 */
export template<std::size_t SlotCount>
class edge_triggered_interrupt_dispatcher final
  : private edge_triggered_callback
{
public:
  /**
   * @brief Subscribe to a pin and provide its slots
   *
   * Configures the pin to trigger on both edges.
   *
   * @warning Never construct the dispatcher from an interrupt service
   * routine. It subscribes to the pin, which throws `hal::subscription_busy`
   * if the pin already has a subscriber.
   *
   * @param p_pin - the pin to share
   * @throws hal::subscription_busy - if the pin already has a subscriber
   * @throws hal::operation_not_supported - if the pin cannot trigger on both
   * edges
   */
  explicit edge_triggered_interrupt_dispatcher(edge_triggered_interrupt& p_pin
                                               [[clang::lifetimebound]])
    : m_subscription(subscribe(p_pin))
  {
  }

  edge_triggered_interrupt_dispatcher(
    edge_triggered_interrupt_dispatcher const&) = delete;
  edge_triggered_interrupt_dispatcher& operator=(
    edge_triggered_interrupt_dispatcher const&) = delete;
  edge_triggered_interrupt_dispatcher(edge_triggered_interrupt_dispatcher&&) =
    delete;
  edge_triggered_interrupt_dispatcher& operator=(
    edge_triggered_interrupt_dispatcher&&) = delete;
  ~edge_triggered_interrupt_dispatcher() = default;

  /**
   * @brief Access a slot
   *
   * @param p_index - index of the slot
   * @return edge_triggered_interrupt_slot& - the slot, which must not outlive
   * the dispatcher
   * @throws hal::out_of_range - if p_index is not less than SlotCount
   */
  [[nodiscard]] edge_triggered_interrupt_slot& operator[](
    std::size_t p_index) & [[clang::lifetimebound]]
  {
    if (p_index >= SlotCount) {
      throw hal::out_of_range(this,
                              { .m_index = p_index, .m_capacity = SlotCount });
    }
    return m_slots[p_index];
  }

  /// A slot of a temporary dispatcher would dangle immediately.
  edge_triggered_interrupt_slot& operator[](std::size_t p_index) && = delete;

  /// @return std::size_t - number of slots
  [[nodiscard]] static constexpr std::size_t size() noexcept
  {
    return SlotCount;
  }

private:
  subscription subscribe(edge_triggered_interrupt& p_pin)
  {
    p_pin.configure({ .trigger = edge_trigger::both });
    return p_pin.on_trigger(*this);
  }

  void callback(bool p_state) noexcept override
  {
    for (auto& slot : m_slots) {
      slot.dispatch(p_state);
    }
  }

  std::array<edge_triggered_interrupt_slot, SlotCount> m_slots{};
  // Last, so the real pin stops calling the dispatcher before the slots are
  // destroyed.
  subscription m_subscription;
};
}  // namespace hal::inline v6
