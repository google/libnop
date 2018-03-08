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

#ifndef LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_
#define LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_

#include <nop/base/encoding.h>
#include <nop/types/optional.h>

namespace nop {

//
// Optional<T> encoding formats:
//
// Empty Optional<T>:
//
// +-----+
// | NIL |
// +-----+
//
// Non-empty Optional<T>
//
// +---//----+
// | ELEMENT |
// +---//----+
//
// Element must be a valid encoding of type T.
//

template <typename T>
struct Encoding<Optional<T>> : EncodingIO<Optional<T>> {
  using Type = Optional<T>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return value ? Encoding<T>::Prefix(value.get()) : EncodingByte::Nil;
  }

  static constexpr std::size_t Size(const Type& value) {
    return value ? Encoding<T>::Size(value.get())
                 : BaseEncodingSize(EncodingByte::Nil);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Nil || Encoding<T>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix, const Type& value,
                                   Writer* writer) {
    if (value)
      return Encoding<T>::WritePayload(prefix, value.get(), writer);
    else
      return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                  Reader* reader) {
    if (prefix == EncodingByte::Nil) {
      value->clear();
    } else {
      T temp;
      auto status = Encoding<T>::ReadPayload(prefix, &temp, reader);
      if (!status)
        return status;

      *value = std::move(temp);
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_OPTIONAL_H_
