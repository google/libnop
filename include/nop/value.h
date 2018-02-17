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

#ifndef LIBNOP_INCLUDE_NOP_VALUE_H_
#define LIBNOP_INCLUDE_NOP_VALUE_H_

#include <nop/base/macros.h>
#include <nop/types/detail/member_pointer.h>

namespace nop {

//
// Value wrappers are structs or classes that have been annotated so that the
// serialization engine understands how to read and write them. They are similar
// to user-defined structures except that only a single member or logical buffer
// pair may be specified. When serialized and deserailized, value wrappers only
// take the wire format of their underlying type, avoiding any additional
// overhead. This is primarily useful for wrapping a bit of constructor,
// operator, and accessor logic around another type.
//
// Annotation is performed by invoking the NOP_VALUE(type, member) macro in the
// body of the struct, class, or template.
//
// Example of a simple float to/from fixed point wrapper template type:
//
//   template <typename Integer, std::size_t FractionalBits_>
//   class Fixed {
//    public:
//     enum : std::size_t {
//       Bits = sizeof(Integer) * 8,
//       FractionalBits = FractionalBits_,
//       IntegralBits = Bits - FractionalBits
//     };
//     enum : std::size_t { Power = 1 << FractionalBits };
//
//     static_assert(std::is_integral<Integer>::value, "");
//     static_assert(FractionalBits < Bits, "");
//
//     constexpr Fixed() = default;
//     constexpr Fixed(const Fixed&) = default;
//     constexpr Fixed(float f) { value_ = std::round(f * Power); }
//
//     Fixed& operator=(const Fixed&) = default;
//     Fixed& operator=(float f) { value_ = std::round(f * Power); }
//
//     constexpr float float_value() const {
//       return static_cast<float>(value_) / Power;
//     }
//
//     explicit constexpr operator float() const { return float_value(); }
//     constexpr Integer value() const { return value_; }
//
//    private:
//     Integer value_;
//     NOP_VALUE(Fixed, value_);
//   };
//

// Defines the member or logical buffer pair to use as the value and effective
// type during serialization/deserailization. This macro must be invoked once
// within the struct/calss definition, preferrably in the private section for
// classes with private data.
#define NOP_VALUE(type, member)            \
  template <typename, typename>            \
  friend struct ::nop::Encoding;           \
  template <typename, typename>            \
  friend struct ::nop::IsValueWrapper;     \
  template <typename, typename>            \
  friend struct ::nop::ValueWrapperTraits; \
  using NOP__VALUE = ::nop::MemberList<_NOP_VALUE_MEMBER(type, member)>

//
// Utility macros used by the macro above.
//

// Generates a pair of template arguments (member pointer type and value) to be
// passed to MemberPointer<MemberPointerType, MemberPointerValue, ...> from the
// given type name and member name.
#define _NOP_VALUE_MEMBER_POINTER(type, member) \
  decltype(&type::member), &type::member

// Generates a MemberPointer type definition, given a type name and a variable
// number of member names. The first member name is handled here, while the
// remaining member names are passed to _NOP_VALUE_MEMBER_POINTER_NEXT for
// recursive expansion.
#define _NOP_VALUE_MEMBER_POINTER_FIRST(type, ...)                 \
  ::nop::MemberPointer<                                            \
      _NOP_VALUE_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
          _NOP_VALUE_MEMBER_POINTER_NEXT(type, _NOP_REST_ARG(__VA_ARGS__))>

// Recursively handles the remaining member names in the template argument list
// for MemberPointer.
#define _NOP_VALUE_MEMBER_POINTER_NEXT(type, ...)                 \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__))                        \
  (, _NOP_VALUE_MEMBER_POINTER(type, _NOP_FIRST_ARG(__VA_ARGS__)) \
         _NOP_DEFER2(__NOP_VALUE_MEMBER_POINTER_NEXT)()(          \
             type, _NOP_REST_ARG(__VA_ARGS__)))(/*done*/)

// Indirection to enable recursive macro expansion of
// _NOP_VALUE_MEMBER_POINTER_NEXT.
#define __NOP_VALUE_MEMBER_POINTER_NEXT() _NOP_VALUE_MEMBER_POINTER_NEXT

// Defines a single MemberPointer type given a type and a member name or group
// of member names.
#define _NOP_VALUE_MEMBER(type, member) \
  NOP_MAP_ARGS(_NOP_VALUE_MEMBER_POINTER_FIRST, (type), member)

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_VALUE_H_
