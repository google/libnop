#ifndef LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
#define LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_

#include <cstddef>
#include <type_traits>

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

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_UTILITY_H_
