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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_ENUM_FLAGS_H_
#define LIBNOP_INCLUDE_NOP_TYPES_ENUM_FLAGS_H_

#include <type_traits>

#include <nop/base/utility.h>

namespace nop {

//
// Utility to make using enum classes as bit flags more convenient and useful.
//
// An enum class type is enabled as a bit flag type by tagging it with the macro
// NOP_ENUM_FLAGS(type). The bitwise and some logical operators can be used on
// values of the tagged enum class without casting. Although this effectively
// relaxes some of the enum class constraints, type safety is preserved by
// restricting operators to arguments of the same flags type.
//
// For example:
//
// namespace MyNamespace {
//   enum class SomeFlags {
//     Foo = 0b001,
//     Bar = 0b010,
//     Baz = 0b100,
//     BuzMask = Bar | Baz,
//   };
//   NOP_ENUM_FLAGS(SomeFlags);
//
//   inline bool IsBuz(SomeFlags value) {
//     return !!(SomeFlags::BuzMask & value);
//   }
// }  // namespace MyNamespace
//
// Note: When tagging an enum class, the macro NOP_ENUM_FLAGS() must be invoked
// in the same namespace as the enum class is originally defined in.
//

// Evaluates to the traits type defined by NOP_ENUM_FLAGS() for the given type
// T. This type alias uses ADL to find the traits type for type T in the
// namespace type T is defined in.
template <typename T>
using EnumFlagsTraits = decltype(NOP__GetEnumFlagsTraits(std::declval<T*>()));

// Evaluates to std::true_type if the given type T has been tagged as an enum
// flags type by NOP_ENUM_FLAGS() or std::false_type otherwise.
template <typename, typename = void>
struct IsEnumFlags : std::false_type {};
template <typename T>
struct IsEnumFlags<T, Void<typename EnumFlagsTraits<T>::Type>>
    : std::true_type {};

// Enable if type T is tagged as an enum flags type.
template <typename T>
using EnableIfEnumFlags = typename std::enable_if<IsEnumFlags<T>::value>::type;

// Macro to tag a given enum class type as an enum flags type. This is
// accomplished by defining a partial specialization of the type
// NOP__ENUM_FLAGS_TRAITS and a function declaration tying the given type to the
// partial specialization for ADL lookup.
#define NOP_ENUM_FLAGS(type)                                                  \
  template <typename>                                                         \
  struct NOP__ENUM_FLAGS_TRAITS;                                              \
  template <>                                                                 \
  struct NOP__ENUM_FLAGS_TRAITS<type> {                                       \
    using Type = type;                                                        \
    static_assert(std::is_enum<type>::value,                                  \
                  "Only enum types may be used for enum flags: type=" #type); \
  };                                                                          \
  inline NOP__ENUM_FLAGS_TRAITS<type> NOP__GetEnumFlagsTraits                 \
      [[gnu::used]] (type*) {                                                 \
    return {};                                                                \
  }

}  // namespace nop

// Operator overloads at global scope enabled only for types tagged as enum
// flags.
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator|(T a, T b) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(static_cast<U>(a) | static_cast<U>(b));
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator&(T a, T b) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator^(T a, T b) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(static_cast<U>(a) ^ static_cast<U>(b));
}

template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T& operator|=(T& a, T b) {
  a = a | b;
  return a;
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T& operator&=(T& a, T b) {
  a = a & b;
  return a;
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T& operator^=(T& a, T b) {
  a = a ^ b;
  return a;
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
T operator~(T value) {
  using U = typename std::underlying_type<T>::type;
  return static_cast<T>(~static_cast<U>(value));
}
template <typename T, typename Enable = ::nop::EnableIfEnumFlags<T>>
bool operator!(T value) {
  using U = typename std::underlying_type<T>::type;
  return !static_cast<U>(value);
}

#endif  // LIBNOP_INCLUDE_NOP_TYPES_ENUM_FLAGS_H_
