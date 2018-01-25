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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_COMPARABLE_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_COMPARABLE_H_

#include <type_traits>

#include <nop/traits/void.h>

namespace nop {

// Trait that determines whether the given types T and U have a defined
// equal comparision operator with type T on the left and type U on the right.
template <typename T, typename U, typename = void>
struct IsComparableEqual : std::false_type {};
template <typename T, typename U>
struct IsComparableEqual<
    T, U, Void<decltype(std::declval<const T&>() == std::declval<const U&>())>>
    : std::true_type {};

// Enable if T is comparable to U for equality.
template <typename T, typename U, typename Return = void>
using EnableIfComparableEqual =
    typename std::enable_if<IsComparableEqual<T, U>::value, Return>::type;

// Trait that determines whether the given types T and U have a defined
// less than comparision operator with type T on the left and type U on the
// right.
template <typename T, typename U, typename = void>
struct IsComparableLess : std::false_type {};
template <typename T, typename U>
struct IsComparableLess<
    T, U, Void<decltype(std::declval<const T&>() < std::declval<const U&>())>>
    : std::true_type {};

// Enable if T is comparable to U for less than inequality.
template <typename T, typename U, typename Return = void>
using EnableIfComparableLess =
    typename std::enable_if<IsComparableLess<T, U>::value, Return>::type;

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_COMPARABLE_H_
