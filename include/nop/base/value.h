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

#ifndef LIBNOP_INCLUDE_NOP_BASE_VALUE_H_
#define LIBNOP_INCLUDE_NOP_BASE_VALUE_H_

#include <nop/base/encoding.h>
#include <nop/base/logical_buffer.h>
#include <nop/base/utility.h>
#include <nop/types/detail/member_pointer.h>

namespace nop {

//
// Value wrapper around type T format:
//
// +-------+
// | VALUE |
// +-------+
//
// VALUE must be a valid encoding of type T.
//

template <typename T>
struct Encoding<T, EnableIfIsValueWrapper<T>> : EncodingIO<T> {
  using MemberList = typename ValueWrapperTraits<T>::MemberList;
  using Pointer = typename ValueWrapperTraits<T>::Pointer;
  using Type = typename Pointer::Type;

  static constexpr EncodingByte Prefix(const T& value) {
    return Encoding<Type>::Prefix(Pointer::Resolve(value));
  }

  static constexpr std::size_t Size(const T& value) {
    return Pointer::Size(value);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<Type>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const T& value, Writer* writer) {
    return Pointer::WritePayload(prefix, value, writer, MemberList{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, T* value,
                                            Reader* reader) {
    return Pointer::ReadPayload(prefix, value, reader, MemberList{});
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_VALUE_H_
