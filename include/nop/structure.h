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

#ifndef LIBNOP_INCLUDE_NOP_STRUCTURE_H_
#define LIBNOP_INCLUDE_NOP_STRUCTURE_H_

#include <type_traits>

#include <nop/base/macros.h>
#include <nop/types/detail/member_pointer.h>

namespace nop {

//
// User-defined structures are structs or classes that have been annotated so
// that the serialization engine understands how to read and write them.
// Annotation is performed by invoking one of the following three macros:
//  * NOP_STRUCTURE in the body of the struct, class, or template.
//  * NOP_EXTERNAL_STRUCTURE at namespace scope matching the type.
//
// Example:
//
//  struct User {
//    std::string name;
//    std::uint8_t age;
//
//    enum class Gender {
//      Other,
//      Female,
//      Male,
//    };
//    nop::Optional<Gender> gender;
//
//    NOP_STRUCTURE(User, name, age, gender);
//  };
//

// Defines the set of members belonging to a type that should be
// serialized/deserialized. This macro must be invoked once within the
// struct/class definition, preferrably in the private section for classes with
// private data.
#define NOP_STRUCTURE(type, ... /*members*/)  \
  template <typename, typename>               \
  friend struct ::nop::Encoding;              \
  template <typename, typename>               \
  friend struct ::nop::HasInternalMemberList; \
  template <typename, typename>               \
  friend struct ::nop::MemberListTraits;      \
  using NOP__MEMBERS = ::nop::MemberList<_NOP_MEMBER_LIST(type, __VA_ARGS__)>

// Defines the set of members belonging to a type that should be
// serialized/deserialized without changing the type itself. This is useful for
// making external library types with public data serializable.
#define NOP_EXTERNAL_STRUCTURE(type, ... /*members*/)                         \
  template <typename, typename>                                               \
  struct NOP__MEMBER_TRAITS;                                                  \
  template <typename T>                                                       \
  struct NOP__MEMBER_TRAITS<T, _NOP_ENABLE_IF_TYPE_MATCH(T, type)> {          \
    using MemberList = ::nop::MemberList<_NOP_MEMBER_LIST(T, __VA_ARGS__)>;   \
  };                                                                          \
  template <template <typename...> class TT, typename... Ts>                  \
  struct NOP__MEMBER_TRAITS<TT<Ts...>,                                        \
                            _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts...)> { \
    using MemberList =                                                        \
        ::nop::MemberList<_NOP_MEMBER_LIST(TT<Ts...>, __VA_ARGS__)>;          \
  };                                                                          \
  template <typename T>                                                       \
  inline _NOP_ENABLE_IF_TYPE_MATCH(T, type, NOP__MEMBER_TRAITS<T, void>)      \
      NOP__GetExternalMemberTraits [[gnu::used]] (T*) {                       \
    return {};                                                                \
  }                                                                           \
  template <template <typename...> class TT, typename... Ts>                  \
  inline _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts...,                       \
                                       NOP__MEMBER_TRAITS<TT<Ts...>, void>)   \
      NOP__GetExternalMemberTraits [[gnu::used]] (TT<Ts...>*) {               \
    return {};                                                                \
  }

// Deprecated. NOP_EXTERNAL_STRUCTURE can handle both type and template
// arguments now. Aliases NOP_EXTERNAL_STRUCTURE for legacy code.
#define NOP_EXTERNAL_TEMPLATE NOP_EXTERNAL_STRUCTURE

// Tags the given type as an unbounded buffer. This macro must be invoked once
// within the struct/class definition, preferrably in the private section next
// to NOP_STRUCTURE.
#define NOP_UNBOUNDED_BUFFER(type) using NOP__UNBOUNDED_BUFFER = type

// Tags the given type as an unbounded buffer without changing the type itself.
#define NOP_EXTERNAL_UNBOUNDED_BUFFER(type)                                    \
  template <typename, typename>                                                \
  struct NOP__UNBOUNDED_BUFFER;                                                \
  template <typename T>                                                        \
  struct NOP__UNBOUNDED_BUFFER<T, _NOP_ENABLE_IF_TYPE_MATCH(T, type)> {};      \
  template <template <typename...> class TT, typename... Ts>                   \
  struct NOP__UNBOUNDED_BUFFER<TT<Ts...>, _NOP_ENABLE_IF_TEMPLATE_MATCH(       \
                                              TT, type, Ts...)> {};            \
  template <typename T>                                                        \
  inline _NOP_ENABLE_IF_TYPE_MATCH(T, type, NOP__UNBOUNDED_BUFFER<T, void>)    \
      NOP__GetUnboundedBuffer [[gnu::used]] (T*) {                             \
    return {};                                                                 \
  }                                                                            \
  template <template <typename...> class TT, typename... Ts>                   \
  inline _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts...,                        \
                                       NOP__UNBOUNDED_BUFFER<TT<Ts...>, void>) \
      NOP__GetUnboundedBuffer [[gnu::used]] (TT<Ts...>*) {                     \
    return {};                                                                 \
  }

//
// Utility macros used by the macros above.
//

// Enable if |type| is a type or elaborated template type matching type T.
#define _NOP_ENABLE_IF_TYPE_MATCH(T, type, ... /*Return*/) \
  std::enable_if_t<decltype(::nop::MatchType<T, type>())::value, ##__VA_ARGS__>

// Enable if |type| is an un-elaborated template type matching type TT.
#define _NOP_ENABLE_IF_TEMPLATE_MATCH(TT, type, Ts, ... /*Return*/)       \
  std::enable_if_t<decltype(::nop::MatchTemplate<TT, type, Ts>())::value, \
                   ##__VA_ARGS__>

// Generates a pair of template arguments (member pointer type and value) to be
// passed to MemberPointer<MemberPointerType, MemberPointerValue, ...> from the
// given type name and member name.
#define _NOP_MEMBER_POINTER(type, member) decltype(&type::member), &type::member

// Generates a MemberPointer type definition, given a type name and a variable
// number of member names. The first member name is handled here, while the
// remaining member names are passed to _NOP_MEMBER_POINTER_NEXT for recursive
// expansion.
#define _NOP_MEMBER_POINTER_FIRST(type, ...)                                  \
  ::nop::MemberPointer<_NOP_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
                           _NOP_MEMBER_POINTER_NEXT(                          \
                               type, _NOP_REST_ARG(__VA_ARGS__))>

// Recursively handles the remaining member names in the template argument list
// for MemberPointer.
#define _NOP_MEMBER_POINTER_NEXT(type, ...)                 \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__))                  \
  (, _NOP_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
         _NOP_DEFER3(__NOP_MEMBER_POINTER_NEXT)()(          \
             type, _NOP_REST_ARG(__VA_ARGS__)))(/*done*/)

// Indirection to enable recursive macro expansion of _NOP_MEMBER_POINTER_NEXT.
#define __NOP_MEMBER_POINTER_NEXT() _NOP_MEMBER_POINTER_NEXT

// Defines a list of MemberPointer types given a type and list of member names.
#define _NOP_MEMBER_LIST(type, ...) \
  NOP_MAP_ARGS(_NOP_MEMBER_POINTER_FIRST, (type), __VA_ARGS__)

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_STRUCTURE_H_
