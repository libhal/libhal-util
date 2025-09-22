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

#include <libhal/functional.hpp>

/**
 * @defgroup StaticCallable Static Callable
 *
 */

namespace hal {
/**
 * @ingroup StaticCallable
 * @brief General class which will be used to allow for signature to be used and
 * then split by the below class.
 *
 * @tparam owner_class class that this static callable belongs to
 * @tparam reference_designator this template argument is used to generate
 * unique static objects for each needed callback
 * @tparam signature function signature to be split up in the static_callable
 * specialization
 */
template<class owner_class, int reference_designator, typename signature>
class static_callable;

/**
 * @ingroup StaticCallable
 * @brief Specialization of static_callable with the return type and arguments
 * split up.
 *
 * @tparam owner_class see static_callable
 * @tparam reference_designator see static_callable
 * @tparam return_t function's return type
 * @tparam args_t function's set of arguments
 */
template<class owner_class,
         int reference_designator,
         typename return_t,
         typename... args_t>
class static_callable<owner_class,
                      reference_designator,
                      return_t(args_t... p_args)>
{
public:
  /**
   * @ingroup StaticCallable
   * @brief Construct a new static callable object
   *
   * @param p_callback - when the static callback function is called, it will
   * call this callback
   */
  explicit static_callable(hal::callback<return_t(args_t... p_args)> p_callback)
  {
    callback = p_callback;
  }

  /**
   * @ingroup StaticCallable
   * @brief Get the static function's address
   *
   * @return auto* - static function's address
   */
  [[nodiscard]] auto* get_handler()
  {
    return &handler;
  }

private:
  /**
   * @ingroup StaticCallable
   * @brief private free function that calls the polymorphic callback. This is
   * the function that can be used as entries in an interrupt vector tables or
   * passed as a function pointer to C API.
   *
   * @param p_args - list of arguments
   * @return return_t - the return type of the callback
   */
  static return_t handler(args_t... p_args)
  {
    return callback(p_args...);
  }

  /**
   * @ingroup StaticCallable
   * @brief the polymorphic callback to be
   *
   */
  inline static hal::callback<return_t(args_t... p_args)> callback;
};
}  // namespace hal
