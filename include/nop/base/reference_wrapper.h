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

#ifndef LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_
#define LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_

#include <functional>

#include <nop/base/encoding.h>

namespace nop {

// std::reference_wrapper<T> encoding forwards to Encoding<T>.

template <typename T>
struct Encoding<std::reference_wrapper<T>>
    : EncodingIO<std::reference_wrapper<T>> {
  using Type = std::reference_wrapper<T>;

  static constexpr EncodingByte Prefix(const Type& value) {
    return Encoding<T>::Prefix(value);
  }

  static constexpr std::size_t Size(const Type& value) {
    return Encoding<T>::Size(value);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return Encoding<T>::Match(prefix);
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte prefix,
                                             const Type& value,
                                             Writer* writer) {
    return Encoding<T>::WritePayload(prefix, value, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte prefix, Type* value,
                                            Reader* reader) {
    return Encoding<T>::ReadPayload(prefix, &value->get(), reader);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_REFERENCE_WRAPPER_H_
