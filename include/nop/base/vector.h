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

#ifndef LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_
#define LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

#include <numeric>
#include <vector>

namespace nop {

//
// std::vector<T> encoding format for non-integral types:
//
// +-----+---------+-----//-----+
// | ARY | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements must be valid encodings of type T.
//
// std::vector<T> encoding format for integral types:
//
// +-----+---------+---//----+
// | BIN | INT64:L | L BYTES |
// +-----+---------+---//----+
//
// Where L = N * sizeof(T).
//
// Elements are stored as direct little-endian representation of the integral
// value; each element is sizeof(T) bytes in size.
//

// Specialization for non-integral types.
template <typename T, typename Allocator>
struct Encoding<std::vector<T, Allocator>, EnableIfNotIntegral<T>>
    : EncodingIO<std::vector<T, Allocator>> {
  using Type = std::vector<T, Allocator>;

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

    // Clear the vector to make sure elements are inserted at the correct
    // indices. Intentionally avoid calling reserve() to prevent abuse from very
    // large size values. Regardless of the size specified in the encoding the
    // bytes remaining in the reader provide a natural upper limit to the number
    // of allocations.
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

// Specialization for integral types.
template <typename T, typename Allocator>
struct Encoding<std::vector<T, Allocator>, EnableIfIntegral<T>>
    : EncodingIO<std::vector<T, Allocator>> {
  using Type = std::vector<T, Allocator>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const SizeType size = value.size() * sizeof(T);
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(size) +
           size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const SizeType length = value.size();
    const SizeType length_bytes = length * sizeof(T);
    auto status = Encoding<SizeType>::Write(length_bytes, writer);
    if (!status)
      return status;

    return writer->Write(value.data(), value.data() + length);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;

    if (size % sizeof(T) != 0)
      return ErrorStatus::InvalidContainerLength;

    const SizeType length = size / sizeof(T);

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous vector sizes.
    status = reader->Ensure(size);
    if (!status)
      return status;

    value->resize(length);
    return reader->Read(value->data(), value->data() + length);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_VECTOR_H_
