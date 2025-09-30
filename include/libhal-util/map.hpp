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

#include <algorithm>

/**
 * @defgroup Map Map
 *
 */
namespace hal {
/**
 * @ingroup Map
 * @brief Map an target value [x] from an input range [a,b] to an output range
 * [c,d].
 *
 * Another term for this is an affine transformation which follows this
 * equation:
 *
 * ```text
 *                / d - c \
 * y = (x - a) * | --------| + c
 *                \ b - a /
 * ```
 *
 * For example:
 *
 *    let x = 5.0
 *    let input_range = [0.0, 10.0]
 *    let output_range = [100.0, 200.0]
 *    The result will be 150.0
 *
 * @param p_target - target value within p_input_range to be mapped to the
 * output range.
 * @param p_input_range - the input range of p_target
 * @param p_output_range - the output range to map p_target to
 * @return constexpr float - value mapped from input range to the output
 * range. The output is clamped to the output range.
 */
[[nodiscard]] constexpr float map(float p_target,
                                  std::pair<float, float> p_input_range,
                                  std::pair<float, float> p_output_range)
{
  /**
   *                / d - c \
   * y = (x - a) * | --------| + c
   *                \ b - a /
   */
  float const x = p_target;
  auto const [a, b] = std::minmax(p_input_range.first, p_input_range.second);
  auto const [c, d] = std::minmax(p_output_range.first, p_output_range.second);

  float const shift_input = x - a;
  float const ratio = (d - c) / (b - a);
  float const y = (shift_input * ratio) + c;
  float const clamped_y = std::clamp(y, c, d);

  return clamped_y;
}
}  // namespace hal
