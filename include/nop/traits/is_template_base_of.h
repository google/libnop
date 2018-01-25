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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_TEMPLATE_BASE_OF_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_TEMPLATE_BASE_OF_H_

#include <type_traits>

namespace nop {

namespace detail {

// Utility to deduce the template type from a derived type.
template <template <typename...> class TT, typename... Ts>
std::true_type DeduceTemplateType(const TT<Ts...>*);
template <template <typename...> class TT>
std::false_type DeduceTemplateType(...);

}  // namespace detail

// Utility determining whether template type TT<...> is a base of type T.
template <template <typename...> class TT, typename T>
using IsTemplateBaseOf =
    decltype(detail::DeduceTemplateType<TT>(std::declval<T*>()));

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_TEMPLATE_BASE_OF_H_
