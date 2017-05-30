#ifndef LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
#define LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_

#include <cstddef>
#include <type_traits>

namespace nop {

// Counting template for recursive template definitions.
template <std::size_t>
struct Index {};

// Trait to determine if all the types in a parameter pack are integral types.
template <typename...>
struct IsIntegral;

template <typename T>
struct IsIntegral<T> : std::is_integral<T> {};

template <typename First, typename... Rest>
struct IsIntegral<First, Rest...>
    : std::integral_constant<
          bool, IsIntegral<First>::value && IsIntegral<Rest...>::value> {};

// Enable if every entry of Types is an integral type.
template <typename... Types>
using EnableIfIntegral =
    typename std::enable_if<IsIntegral<Types...>::value>::type;

// Enable if any entry of Types is not an integral type.
template <typename... Types>
using EnableIfNotIntegral =
    typename std::enable_if<!IsIntegral<Types...>::value>::type;

// Utility type for SFINAE expression evaluation.
template <typename... Ts>
using Void = void;

// Utility to deduce the template type from a derived type.
template <template <typename...> class TT, typename... Ts>
std::true_type DeduceTemplateType(const TT<Ts...>*);
template <template <typename...> class TT>
std::false_type DeduceTemplateType(...);

// Utility determining whether template type TT<...> is a base of type T.
template <template <typename...> class TT, typename T>
using IsTemplateBaseOf = decltype(DeduceTemplateType<TT>(std::declval<T*>()));

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
