/*
 * Copyright 2019 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_DETECTED_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_DETECTED_H_

#include <type_traits>

#include <nop/traits/void.h>

namespace nop {

namespace detail {

template <template <typename...> class Trait, typename Expression,
          typename... Args>
struct IsDetectedType : std::false_type {};

template <template <typename...> class Trait, typename... Args>
struct IsDetectedType<Trait, Void<Trait<Args...>>, Args...> : std::true_type {};

}  // namespace detail

// Trait that determines whether the given Trait template is successfully
// evaluated with the given Args. This makes it possible to express SFINAE
// detection expressions as templated aliases instead of full structure
// definitions with partial specializations.
//
// For example, a callable detector may be implemented as follows:
//
// template <typename Callable, typename... Args>
// using CallableTest =
//     decltype(std::declval<Callable>()(std::declval<Args>()...));
//
// template <typename Callable, typename... Args>
// using IsCallable = IsDetected<CallableTest, Callable, Args...>;
//
template <template <typename...> class Trait, typename... Args>
using IsDetected = detail::IsDetectedType<Trait, void, Args...>;

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_DETECTED_H_
