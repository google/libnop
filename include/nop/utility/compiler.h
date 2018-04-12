/*
 * Copyright 2017 The Native Object Protocols Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_COMPILER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_COMPILER_H_

#define NOP_GCC_VERSION \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// Compatability with non-clang compilers.
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif

// Test for fallthrough support.
#if __has_cpp_attribute(clang::fallthrough)
#define NOP_FALLTHROUGH [[clang::fallthrough]]
#elif NOP_GCC_VERSION >= 70000
#define NOP_FALLTHROUGH [[fallthrough]]
#else
#define NOP_FALLTHROUGH
#endif

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_COMPILER_H_
