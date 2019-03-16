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

#ifndef LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
#define LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_

#include <cstddef>
#include <type_traits>

#include <nop/traits/is_template_base_of.h>
#include <nop/traits/void.h>

namespace nop {

// Counting template for recursive template definitions.
template <std::size_t>
struct Index {};

// Passthrough type.
template <typename T>
using Identity = T;

// Trait to determine if all the types in a parameter pack are integral types.
template <typename...>
struct IsIntegral;
template <typename T>
struct IsIntegral<T> : std::is_integral<T> {};
template <typename First, typename... Rest>
struct IsIntegral<First, Rest...>
    : std::integral_constant<bool, IsIntegral<First>::value &&
                                       IsIntegral<Rest...>::value> {};

// Trait to determine if all types in a parameter pack are arithmetic types.
template <typename...>
struct IsArithmetic;
template <typename T>
struct IsArithmetic<T> : std::is_arithmetic<T> {};
template <typename First, typename... Rest>
struct IsArithmetic<First, Rest...>
    : std::integral_constant<bool, IsArithmetic<First>::value &&
                                       IsArithmetic<Rest...>::value> {};

// Enable if every entry of Types is an integral type.
template <typename... Types>
using EnableIfIntegral =
    typename std::enable_if<IsIntegral<Types...>::value>::type;

// Enable if any entry of Types is not an integral type.
template <typename... Types>
using EnableIfNotIntegral =
    typename std::enable_if<!IsIntegral<Types...>::value>::type;

// Enable if every entry of Types is an arithmetic type.
template <typename... Types>
using EnableIfArithmetic =
    typename std::enable_if<IsArithmetic<Types...>::value>::type;

// Enable if every entry of Types is not an arithmetic type.
template <typename... Types>
using EnableIfNotArithmetic =
    typename std::enable_if<!IsArithmetic<Types...>::value>::type;

template <typename T, typename U>
using EnableIfConvertible =
    typename std::enable_if<std::is_convertible<T, U>::value>::type;

// Utility type to retrieve the first type in a parameter pack.
template <typename...>
struct FirstType {};
template <typename First, typename... Rest>
struct FirstType<First, Rest...> {
  using Type = First;
};
template <typename... Ts>
using First = typename FirstType<Ts...>::Type;

// Determines the value type and extent of C/C++ array types.
template <typename T>
struct ArrayTraits : std::false_type {};

template <typename T, std::size_t Length_>
struct ArrayTraits<T[Length_]> : std::true_type {
  enum : std::size_t { Length = Length_ };
  using ElementType = T;
  using Type = T[Length];
};
template <typename T, std::size_t Length_>
struct ArrayTraits<std::array<T, Length_>> : std::true_type {
  enum : std::size_t { Length = Length_ };
  using ElementType = T;
  using Type = std::array<T, Length>;
};
template <typename T, std::size_t Length_>
struct ArrayTraits<const std::array<T, Length_>> : std::true_type {
  enum : std::size_t { Length = Length_ };
  using ElementType = T;
  using Type = const std::array<T, Length>;
};

// Enable if T is an array type.
template <typename T, typename Return = void>
using EnableIfArray =
    typename std::enable_if<ArrayTraits<T>::value, Return>::type;

// Logical AND over template parameter pack.
template <typename... T>
struct And : std::true_type {};
template <typename A>
struct And<A> : A {};
template <typename A, typename B>
struct And<A, B> : std::integral_constant<bool, A::value && B::value> {};
template <typename A, typename B, typename... Rest>
struct And<A, B, Rest...> : And<A, And<B, Rest...>> {};

// Logical OR over template parameter pack.
template <typename... T>
struct Or : std::true_type {};
template <typename A>
struct Or<A> : A {};
template <typename A, typename B>
struct Or<A, B> : std::integral_constant<bool, A::value || B::value> {};
template <typename A, typename B, typename... Rest>
struct Or<A, B, Rest...> : Or<A, Or<B, Rest...>> {};

// Utility to determine whether a set of one or more types is a true set,
// containing no duplicates, according to the given comparison template. The
// comparison template must accept two type arguments and evaluate to true if
// its arguments are the same according to the intended criteria.
//
// An example using integer types and std::is_same for comparison:
//
//  static_assert(IsUnique<std::is_same, int, short, int>::value,
//                "Types in set are not unique!");
//
template <template <typename, typename> class, typename...>
struct IsUnique;
template <template <typename, typename> class Same>
struct IsUnique<Same> : std::true_type {};
template <template <typename, typename> class Same, typename T>
struct IsUnique<Same, T> : std::true_type {};
template <template <typename, typename> class Same, typename First,
          typename Second>
struct IsUnique<Same, First, Second>
    : std::integral_constant<bool, !Same<First, Second>::value> {};
template <template <typename, typename> class Same, typename First,
          typename Second, typename... Rest>
struct IsUnique<Same, First, Second, Rest...>
    : And<IsUnique<Same, First, Second>, IsUnique<Same, First, Rest...>,
          IsUnique<Same, Second, Rest...>> {};

// Utility to determine whether all types in a set compare the same using the
// given comparison.
//
// An example using integer types and std::is_same for comparison:
//
//  static_assert(IsSame<std::is_same, int, short, int>::value,
//                "Types in set are not the same!");
//
template <template <typename, typename> class, typename...>
struct IsSame;
template <template <typename, typename> class Same>
struct IsSame<Same> : std::true_type {};
template <template <typename, typename> class Same, typename T>
struct IsSame<Same, T> : std::true_type {};
template <template <typename, typename> class Same, typename First,
          typename... Rest>
struct IsSame<Same, First, Rest...> : And<Same<First, Rest>...> {};

// Utility types for SFINAE expression matching either regular or template
// types.
template <typename T, typename U>
std::is_same<T, U> MatchType();

template <template <typename...> class TT, template <typename...> class UU,
          typename... Ts>
std::is_same<TT<Ts...>, UU<Ts...>> MatchTemplate();

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
