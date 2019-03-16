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

#ifndef LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_
#define LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_

#include <array>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nop/base/logical_buffer.h>
#include <nop/base/members.h>
#include <nop/base/table.h>
#include <nop/base/utility.h>
#include <nop/types/optional.h>
#include <nop/types/result.h>
#include <nop/types/variant.h>

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
template <typename A, typename B, typename Return = void>
using EnableIfFungible =
    typename std::enable_if<IsFungible<A, B>::value, Return>::type;

// Compares two function types to see if the return types and arguments are
// fungible.
template <typename ReturnA, typename ReturnB, typename... ArgsA,
          typename... ArgsB>
struct IsFungible<ReturnA(ArgsA...), ReturnB(ArgsB...),
                  std::enable_if_t<sizeof...(ArgsA) == sizeof...(ArgsB)>>
    : And<IsFungible<std::decay_t<ReturnA>, std::decay_t<ReturnB>>,
          IsFungible<std::decay_t<ArgsA>, std::decay_t<ArgsB>>...> {};

// Compares two std::arrays to see if the element types are fungible.
template <typename A, typename B, std::size_t Size>
struct IsFungible<std::array<A, Size>, std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares two C arrays to see if the element types are fungible. Sizes are
// explicitly compared to avoid falling back on the base IsFungible type which
// would decay A and B to pointers and fail to compare different array sizes
// correctly.
template <typename A, typename B, std::size_t SizeA, std::size_t SizeB>
struct IsFungible<A[SizeA], B[SizeB]>
    : And<IsFungible<std::decay_t<A>, std::decay_t<B>>,
          std::integral_constant<bool, SizeA == SizeB>> {};

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
// fungible. Fungible tuples must have the same number of elements.
template <typename... A, typename... B>
struct IsFungible<std::tuple<A...>, std::tuple<B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<std::decay_t<A>, std::decay_t<B>>...> {};
template <typename... A, typename... B>
struct IsFungible<std::tuple<A...>, std::tuple<B...>,
                  std::enable_if_t<sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

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

// Compares std::vector with an n-element std::tuple to see if every element of
// the tuple is fugible with the non-integral vector element type.
template <typename T, typename Allocator, typename... Ts>
struct IsFungible<std::vector<T, Allocator>, std::tuple<Ts...>,
                  EnableIfNotIntegral<T>> : And<IsFungible<T, Ts>...> {};
template <typename T, typename Allocator, typename... Ts>
struct IsFungible<std::tuple<Ts...>, std::vector<T, Allocator>,
                  EnableIfNotIntegral<T>> : And<IsFungible<T, Ts>...> {};

// Compares std::array with an n-element std::tuple to see if every element of
// the tuple is fugible with the non-integral array element type.
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    std::array<T, Size>, std::tuple<Ts...>,
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    std::tuple<Ts...>, std::array<T, Size>,
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};

// Compares C array with an n-element std::tuple to see if every element of
// the tuple is fugible with the non-integral C array element type.
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    T[Size], std::tuple<Ts...>,
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};
template <typename T, std::size_t Size, typename... Ts>
struct IsFungible<
    std::tuple<Ts...>, T[Size],
    std::enable_if_t<Size == sizeof...(Ts) && !std::is_integral<T>::value>>
    : And<IsFungible<T, Ts>...> {};

// Compares std::vector and std::array to see if the element types are
// fungible.
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::vector<A, Allocator>, std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::array<A, Size>, std::vector<B, Allocator>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares C array and std::vector to see if the elements types are fungible.
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<A[Size], std::vector<B, Allocator>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, typename Allocator, std::size_t Size>
struct IsFungible<std::vector<A, Allocator>, B[Size]>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares C array and std::array to see if the element types are fungible.
template <typename A, typename B, std::size_t Size>
struct IsFungible<A[Size], std::array<B, Size>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};
template <typename A, typename B, std::size_t Size>
struct IsFungible<std::array<A, Size>, B[Size]>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Result<ErrorEnum, A> and Result<ErrorEnum, B> to see if A and B are
// fungible. ErrorEnum must be the same between fungible Result types because
// different enum types are not fungible.
template <typename ErrorEnum, typename A, typename B>
struct IsFungible<Result<ErrorEnum, A>, Result<ErrorEnum, B>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Optional<A> and Optional<B> to see if A and B are fungible.
template <typename A, typename B>
struct IsFungible<Optional<A>, Optional<B>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Entry<A> and Entry<B> to see if A and B are fungible.
template <typename A, typename B, std::uint64_t Id, typename Type>
struct IsFungible<Entry<A, Id, Type>, Entry<B, Id, Type>>
    : IsFungible<std::decay_t<A>, std::decay_t<B>> {};

// Compares Variant<A...> and Variant<B...> to see if every member of A is
// fungible with every corresponding member of B.
template <typename... A, typename... B>
struct IsFungible<Variant<A...>, Variant<B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<A, B>...> {};
template <typename... A, typename... B>
struct IsFungible<Variant<A...>, Variant<B...>,
                  std::enable_if_t<sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares two LogicalBuffers to see if they are fungible.
template <typename A, typename B, typename SizeTypeA, typename SizeTypeB,
          bool IsUnboundedA, bool IsUnboundedB, typename EnabledA,
          typename EnabledB>
struct IsFungible<LogicalBuffer<A, SizeTypeA, IsUnboundedA, EnabledA>,
                  LogicalBuffer<B, SizeTypeB, IsUnboundedB, EnabledB>>
    : IsFungible<A, B> {};

// Compares a std::vector and a LogicalBuffer to see if they are fungible.
template <typename A, typename B, typename AllocatorA, typename SizeTypeB,
          bool IsUnboundedB, typename EnabledB>
struct IsFungible<std::vector<A, AllocatorA>,
                  LogicalBuffer<B, SizeTypeB, IsUnboundedB, EnabledB>>
    : IsFungible<std::vector<A, AllocatorA>, B> {};
template <typename A, typename B, typename SizeTypeA, bool IsUnboundedA,
          typename EnabledA, typename AllocatorB>
struct IsFungible<LogicalBuffer<A, SizeTypeA, IsUnboundedA, EnabledA>,
                  std::vector<B, AllocatorB>>
    : IsFungible<A, std::vector<B, AllocatorB>> {};

// Compares MemberList<A...> and MemberList<B...> to see if every
// MemberPointer::Type in A is fungible with every MemberPointer::Type in B.
template <typename... A, typename... B>
struct IsFungible<MemberList<A...>, MemberList<B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<typename A::Type, typename B::Type>...> {};
template <typename... A, typename... B>
struct IsFungible<MemberList<A...>, MemberList<B...>,
                  std::enable_if_t<sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares user-defined types A and B to see if every member is fungible.
template <typename A, typename B>
struct IsFungible<
    A, B, std::enable_if_t<HasMemberList<A>::value && HasMemberList<B>::value>>
    : IsFungible<typename MemberListTraits<A>::MemberList,
                 typename MemberListTraits<B>::MemberList> {};

// Compares user-defined value wrapper types A and B to see if the values are
// fungible.
template <typename A, typename B>
struct IsFungible<
    A, B,
    std::enable_if_t<IsValueWrapper<A>::value && IsValueWrapper<B>::value>>
    : IsFungible<typename ValueWrapperTraits<A>::MemberList,
                 typename ValueWrapperTraits<B>::MemberList> {};

// Compares user-defined value wrapper type A and non-wrapper type B and vice
// versa to see if the wrapped and non-wrapped types are fungible.
template <typename A, typename B>
struct IsFungible<
    A, B,
    std::enable_if_t<IsValueWrapper<A>::value && !IsValueWrapper<B>::value>>
    : IsFungible<typename ValueWrapperTraits<A>::Pointer::Type, B> {};
template <typename A, typename B>
struct IsFungible<
    A, B,
    std::enable_if_t<!IsValueWrapper<A>::value && IsValueWrapper<B>::value>>
    : IsFungible<A, typename ValueWrapperTraits<B>::Pointer::Type> {};

template <typename Hash, typename... A, typename... B>
struct IsFungible<EntryList<Hash, A...>, EntryList<Hash, B...>,
                  std::enable_if_t<sizeof...(A) == sizeof...(B)>>
    : And<IsFungible<typename A::Type, typename B::Type>...> {};
template <typename HashA, typename HashB, typename... A, typename... B>
struct IsFungible<EntryList<HashA, A...>, EntryList<HashB, B...>,
                  std::enable_if_t<!std::is_same<HashA, HashB>::value ||
                                   sizeof...(A) != sizeof...(B)>>
    : std::false_type {};

// Compares table types A and B to see if every member is fungible.
template <typename A, typename B>
struct IsFungible<
    A, B, std::enable_if_t<HasEntryList<A>::value && HasEntryList<B>::value>>
    : IsFungible<typename EntryListTraits<A>::EntryList,
                 typename EntryListTraits<B>::EntryList> {};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TRAITS_IS_FUNGIBLE_H_
