# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 3.15)

project(libhal-util LANGUAGES CXX)

set(TEST_SOURCES_LIST
  tests/inert_drivers/inert_accelerometer.test.cpp
  tests/inert_drivers/inert_adc.test.cpp
  tests/inert_drivers/inert_dac.test.cpp
  tests/inert_drivers/inert_distance_sensor.test.cpp
  tests/inert_drivers/inert_gyroscope.test.cpp
  tests/inert_drivers/inert_input_pin.test.cpp
  tests/inert_drivers/inert_interrupt_pin.test.cpp
  tests/inert_drivers/inert_magnetometer.test.cpp
  tests/inert_drivers/inert_motor.test.cpp
  tests/inert_drivers/inert_pwm.test.cpp
  tests/inert_drivers/inert_output_pin.test.cpp
  tests/inert_drivers/inert_rotation_sensor.test.cpp
  tests/inert_drivers/inert_steady_clock.test.cpp
  tests/inert_drivers/inert_temperature_sensor.test.cpp
  tests/inert_drivers/inert_timer.test.cpp
  tests/as_bytes.test.cpp
  tests/atomic_spin_lock.test.cpp
  tests/can.test.cpp
  tests/bit.test.cpp
  tests/bit.functions.test.cpp
  tests/enum.test.cpp
  tests/i2c.test.cpp
  tests/input_pin.test.cpp
  tests/interrupt_pin.test.cpp
  tests/map.test.cpp
  tests/math.test.cpp
  tests/move_interceptor.test.cpp
  tests/output_pin.test.cpp
  tests/overflow_counter.test.cpp
  tests/serial.test.cpp
  tests/spi.test.cpp
  tests/static_callable.test.cpp
  tests/serial_coroutines.test.cpp
  tests/static_list.test.cpp
  tests/steady_clock.test.cpp
  tests/streams.test.cpp
  tests/timeout.test.cpp
  tests/bit_bang_i2c.test.cpp
  tests/bit_bang_spi.test.cpp
  tests/units.test.cpp
  tests/main.test.cpp
)

set(SOURCES_LIST
  src/steady_clock.cpp
  src/streams.cpp
  src/atomic_spin_lock.cpp
  src/bit_bang_i2c.cpp
  src/bit_bang_spi.cpp

  # TODO(#43): Add this back
  # src/inverter.cpp
)

if(NOT ${CMAKE_CROSSCOMPILING})
  libhal_unit_test(
    SOURCES
    ${SOURCES_LIST}
    ${TEST_SOURCES_LIST}

    PACKAGES
    libhal

    LINK_LIBRARIES
    libhal::libhal
  )
endif()

libhal_make_library(
  LIBRARY_NAME libhal-util

  SOURCES
  ${SOURCES_LIST}

  PACKAGES
  libhal

  LINK_LIBRARIES
  libhal::libhal
)
