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

#include <nop/base/macros.h>
#include <nop/types/detail/member_pointer.h>

namespace nop {

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
         _NOP_DEFER2(__NOP_MEMBER_POINTER_NEXT)()(          \
             type, _NOP_REST_ARG(__VA_ARGS__)))(/*done*/)

// Indirection to enable recursive macro expansion of _NOP_MEMBER_POINTER_NEXT.
#define __NOP_MEMBER_POINTER_NEXT() _NOP_MEMBER_POINTER_NEXT

// Defines a list of MemberPointer types given a type and list of member names.
#define NOP_MEMBER_LIST(type, ...) \
  NOP_MAP_ARGS(_NOP_MEMBER_POINTER_FIRST, (type), __VA_ARGS__)

// Defines the set of members belonging to a type that should be
// serialized/deserialized. This macro should be called once within the
// struct/class definition, preferrably in the private section for classes with
// private data.
#define NOP_MEMBERS(type, ... /*members*/)    \
  template <typename, typename>               \
  friend struct ::nop::Encoding;              \
  template <typename, typename>               \
  friend struct ::nop::HasInternalMemberList; \
  template <typename, typename>               \
  friend struct ::nop::MemberListTraits;      \
  using NOP__MEMBERS = ::nop::MemberList<NOP_MEMBER_LIST(type, __VA_ARGS__)>

// Defines the set of members belonging to a type that should be
// serialized/deserialized without changing the type itself. This is useful for
// making external library types with public data serializable.
#define NOP_STRUCTURE(type, ... /*members*/)                                  \
  template <typename>                                                         \
  struct NOP__MEMBER_TRAITS;                                                  \
  template <>                                                                 \
  struct NOP__MEMBER_TRAITS<type> {                                           \
    using MemberList = ::nop::MemberList<NOP_MEMBER_LIST(type, __VA_ARGS__)>; \
  };                                                                          \
  NOP__MEMBER_TRAITS<type> __attribute__((used))                              \
      NOP__GetExternalMemberTraits(type*)

// Similar to NOP_STRUCTURE but for template types.
#define NOP_TEMPLATE(type, ... /*members*/)                           \
  template <typename>                                                 \
  struct NOP__MEMBER_TRAITS;                                          \
  template <typename... Ts>                                           \
  struct NOP__MEMBER_TRAITS<type<Ts...>> {                            \
    using MemberList =                                                \
        ::nop::MemberList<NOP_MEMBER_LIST(type<Ts...>, __VA_ARGS__)>; \
  };                                                                  \
  template <typename... Ts>                                           \
  NOP__MEMBER_TRAITS<type<Ts...>> __attribute__((used))               \
      NOP__GetExternalMemberTraits(type<Ts...>*)

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_STRUCTURE_H_
