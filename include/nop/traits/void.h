/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_VOID_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_VOID_H_

namespace nop {

namespace detail {

template <typename... Ts>
struct MakeVoid {
  using Type = void;
};

}  // namespace detail

// Utility type for SFINAE expression evaluation. C++14 equivalent to C++17
// std::void_t.
//
// This approach addresses the issue with older C++14 compilers described in:
// http://open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#1558
template <typename... Ts>
using Void = typename detail::MakeVoid<Ts...>::Type;

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_VOID_H_
