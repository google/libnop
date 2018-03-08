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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ENUM_H_
#define LIBNOP_INCLUDE_NOP_BASE_ENUM_H_

#include <type_traits>

#include <nop/base/encoding.h>

namespace nop {

// Enable if T is an enumeration type.
template <typename T, typename ReturnType = void>
using EnableIfEnum =
    typename std::enable_if<std::is_enum<T>::value, ReturnType>::type;

//
// enum encoding format matches the encoding format of the underlying integer
// type.
//

template <typename T>
struct Encoding<T, EnableIfEnum<T>> : EncodingIO<T> {
  static constexpr EncodingByte Prefix(const T& value) {
    return Encoding<IntegerType>::Prefix(static_cast<IntegerType>(value));
  }

  static constexpr std::size_t Size(const T& value) {
    return Encoding<IntegerType>::Size(static_cast<IntegerType>(value));
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<IntegerType>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const T& value, Writer* writer) {
    return Encoding<IntegerType>::WritePayload(
        prefix, reinterpret_cast<const IntegerType&>(value), writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, T* value,
                                            Reader* reader) {
    return Encoding<IntegerType>::ReadPayload(
        prefix, reinterpret_cast<IntegerType*>(value), reader);
  }

 private:
  using IntegerType = typename std::underlying_type<T>::type;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ENUM_H_
