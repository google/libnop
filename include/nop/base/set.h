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

#ifndef LIBNOP_INCLUDE_NOP_BASE_SET_H_
#define LIBNOP_INCLUDE_NOP_BASE_SET_H_

#include <nop/base/encoding.h>

#include <numeric>
#include <set>
#include <unordered_set>

namespace nop {

//
// std::set<Key> and std::unordered_set<Key> encoding format:
//
// +-----+---------+---//---+
// | SET | INT64:N | N KEYS |
// +-----+---------+---//---+
//
// Elements must be valid encodings of type T.
//

template <typename Key, typename Compare, typename Allocator>
struct Encoding<std::set<Key, Compare, Allocator>>
    : EncodingIO<std::set<Key, Compare, Allocator>> {
  using Type = std::set<Key, Compare, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(value.cbegin(), value.cend(), 0U,
                           [](const std::size_t& sum, const Key& element) {
                             return sum + Encoding<Key>::Size(element);
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

    for (const Key& element : value) {
      status = Encoding<Key>::Write(element, writer);
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
      Key element;
      status = Encoding<Key>::Read(&element, reader);
      if (!status)
        return status;

      value->insert(std::move(element));
    }

    return {};
  }
};

template <typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct Encoding<std::unordered_set<Key, Hash, KeyEqual, Allocator>>
    : EncodingIO<std::unordered_set<Key, Hash, KeyEqual, Allocator>> {
  using Type = std::unordered_set<Key, Hash, KeyEqual, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(value.cbegin(), value.cend(), 0U,
                           [](const std::size_t& sum, const Key& element) {
                             return sum + Encoding<Key>::Size(element);
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

    for (const Key& element : value) {
      status = Encoding<Key>::Write(element, writer);
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
      Key element;
      status = Encoding<Key>::Read(&element, reader);
      if (!status)
        return status;

      value->insert(std::move(element));
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_SET_H_
