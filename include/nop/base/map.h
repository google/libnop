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

#ifndef LIBNOP_INCLUDE_NOP_BASE_MAP_H_
#define LIBNOP_INCLUDE_NOP_BASE_MAP_H_

#include <map>
#include <numeric>
#include <unordered_map>

#include <nop/base/encoding.h>

namespace nop {

//
// std::map<Key, T> and std::unordered_map<Key, T> encoding format:
//
// +-----+---------+--------//---------+
// | MAP | INT64:N | N KEY/VALUE PAIRS |
// +-----+---------+--------//---------+
//
// Each pair must be a valid encoding of Key followed by a valid encoding of T.
//

template <typename Key, typename T, typename Compare, typename Allocator>
struct Encoding<std::map<Key, T, Compare, Allocator>>
    : EncodingIO<std::map<Key, T, Compare, Allocator>> {
  using Type = std::map<Key, T, Compare, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Map;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(
               value.cbegin(), value.cend(), 0U,
               [](const std::size_t& sum, const std::pair<Key, T>& element) {
                 return sum + Encoding<Key>::Size(element.first) +
                        Encoding<T>::Size(element.second);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Map;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const std::pair<Key, T>& element : value) {
      status = Encoding<Key>::Write(element.first, writer);
      if (!status)
        return status;

      status = Encoding<T>::Write(element.second, writer);
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
      std::pair<Key, T> element;
      status = Encoding<Key>::Read(&element.first, reader);
      if (!status)
        return status;

      status = Encoding<T>::Read(&element.second, reader);
      if (!status)
        return status;

      value->emplace(std::move(element));
    }

    return {};
  }
};

template <typename Key, typename T, typename Hash, typename KeyEqual,
          typename Allocator>
struct Encoding<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>>
    : EncodingIO<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> {
  using Type = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Map;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) +
           std::accumulate(
               value.cbegin(), value.cend(), 0U,
               [](const std::size_t& sum, const std::pair<Key, T>& element) {
                 return sum + Encoding<Key>::Size(element.first) +
                        Encoding<T>::Size(element.second);
               });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Map;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(value.size(), writer);
    if (!status)
      return status;

    for (const std::pair<Key, T>& element : value) {
      status = Encoding<Key>::Write(element.first, writer);
      if (!status)
        return status;

      status = Encoding<T>::Write(element.second, writer);
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
      std::pair<Key, T> element;
      status = Encoding<Key>::Read(&element.first, reader);
      if (!status)
        return status;

      status = Encoding<T>::Read(&element.second, reader);
      if (!status)
        return status;

      value->emplace(std::move(element));
    }

    return {};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_MAP_H_
