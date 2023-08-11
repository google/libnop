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

#ifndef LIBNOP_INCLUDE_NOP_BASE_DEQUE_H_
#define LIBNOP_INCLUDE_NOP_BASE_DEQUE_H_

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

#include <deque>
#include <numeric>

namespace nop {

//
// std::deque<T> encoding format:
//
// +-------+---------+-----//-----+
// | DEQUE | INT64:N | N ELEMENTS |
// +-------+---------+-----//-----+
//
// Elements must be valid encodings of type T.
//

template <typename T, typename Allocator>
struct Encoding<std::deque<T, Allocator>>
    : EncodingIO<std::deque<T, Allocator>> {
  using Type = std::deque<T, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(value.cbegin(), value.cend(), 0U,
                           [](const std::size_t& sum, const T& element) {
                             return sum + Encoding<T>::Size(element);
                           });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const T& element : value) {
      status = Encoding<T>::Write(element, writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    value->clear();
    for (SizeType i = 0; i < size; i++) {
      T element;
      status = Encoding<T>::Read(&element, reader);
      if (!status)
        return status;

      value->push_back(std::move(element));
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_DEQUE_H_
