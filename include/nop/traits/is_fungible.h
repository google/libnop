#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_

#include <array>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nop/base/utility.h>

// This header defines rules for which types have equivalent encodings. Types
// with equivalent encodings my be legally substituted during serialization and
// interface binding to enable greater flexibility in user code.
//
// For example, std::vector<T> and std::array<T> of the same type T encode using
// the same format.

namespace nop {

// Compares A and B for fungibility. This base type determines convertibility by
// equivalence of the underlying types of A and B. Specializations of this type
// handle the rules for which complex types are fungible.
template <typename A, typename B, typename Enabled = void>
struct IsFungible : std::is_same<std::decay_t<A>, std::decay_t<B>> {};

// Enable if A and B are fungible.
template <typename A, typename B>
using EnableIfFungible = typename std::enable_if<IsFungible<A, B>::value>::type;

// Compares two function types to see if the return types and arguments are
// fungible.
template <typename ReturnA, typename ReturnB, typename... ArgsA,
          typename... ArgsB>
struct IsFungible<ReturnA(ArgsA...), ReturnB(ArgsB...)>
    : And<IsFungible<std::decay_t<ReturnA>, std::decay_t<ReturnB>>,
          IsFungible<std::decay_t<ArgsA>, std::decay_t<ArgsB>>...> {};

// Compares two std::arrays to see if the element types are fungible.
template <typename A, typename B, std::size_t Size>
struct IsFungible<std::array<A, Size>, std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares two std::vectors to see if the element types are fungible.
template <typename A, typename B, typename AllocatorA, typename AllocatorB>
struct IsFungible<std::vector<A, AllocatorA>, std::vector<B, AllocatorB>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares two std::maps to see if the element types are fungible.
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::map<KeyA, ValueA, AnyA...>,
                  std::map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};

// Compares two std::unordered_maps to see if the element types are fungible.
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::unordered_map<KeyA, ValueA, AnyA...>,
                  std::unordered_map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};

// Compares a std::map and a std::unordered_map to see if the element types are
// fungible.
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::map<KeyA, ValueA, AnyA...>,
                  std::unordered_map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};
template <typename KeyA, typename ValueA, typename KeyB, typename ValueB,
          typename... AnyA, typename... AnyB>
struct IsFungible<std::unordered_map<KeyA, ValueA, AnyA...>,
                  std::map<KeyB, ValueB, AnyB...>>
    : And<IsFungible<std::decay_t<KeyA>, std::decay_t<KeyB>>,
          IsFungible<std::decay_t<ValueA>, std::decay_t<ValueB>>> {};

// Compares two std::tuples to see if the corresponding elements are
// fungible. The tuples must have the same number of elements.
template <typename... A, typename... B>
struct IsFungible<std::tuple<A...>, std::tuple<B...>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<B>>...> {};

// Compares two std::pairs to see if the corresponding elements are fungible.
template <typename A, typename B, typename C, typename D>
struct IsFungible<std::pair<A, B>, std::pair<C, D>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<C>>,
          IsFungible<std::decay_t<B>, std::decay_t<D>>> {};

// Compares std::pair with a two-element std::tuple to see if the corresponding
// elements are fungible.
template <typename A, typename B, typename C, typename D>
struct IsFungible<std::pair<A, B>, std::tuple<C, D>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<C>>,
          IsFungible<std::decay_t<B>, std::decay_t<D>>> {};
template <typename A, typename B, typename C, typename D>
struct IsFungible<std::tuple<A, B>, std::pair<C, D>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<C>>,
          IsFungible<std::decay_t<B>, std::decay_t<D>>> {};

// Compares std::vector and std::array to see if the element types are
// fungible.
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::vector<A, Allocator>, std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::array<A, Size>, std::vector<B, Allocator>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

}  // namepsace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_
