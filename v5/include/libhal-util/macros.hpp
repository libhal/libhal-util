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

#pragma once

#if defined(__clang__)
#define SUPPRESS_DEPRECATED_START                                              \
  _Pragma("clang diagnostic push")                                             \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define SUPPRESS_DEPRECATED_END _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define SUPPRESS_DEPRECATED_START                                              \
  _Pragma("GCC diagnostic push")                                               \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define SUPPRESS_DEPRECATED_END _Pragma("GCC diagnostic pop")
#else
#define SUPPRESS_DEPRECATED_START
#define SUPPRESS_DEPRECATED_END
#endif
