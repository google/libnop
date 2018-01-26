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

#ifndef LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_
#define LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_

#include <array>
#include <numeric>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

namespace nop {

//
// std::array<T, N> and T[N] encoding format for non-integral types:
//
// +-----+---------+-----//-----+
// | ARY | INT64:N | N ELEMENTS |
// +-----+---------+-----//-----+
//
// Elements must be valid encodings of type T.
//
// std::array<T, N> and T[N] encoding format for integral types:
//
// +-----+---------+---//----+
// | BIN | INT64:L | L BYTES |
// +-----+---------+---//----+
//
// Where L = N * sizeof(T).
//
// Elements are stored as direct little-endian representation of the integral
// value, each element is sizeof(T) bytes in size.
//

template <typename T, std::size_t Length>
struct Encoding<std::array<T, Length>, EnableIfNotIntegral<T>>
    : EncodingIO<std::array<T, Length>> {
  using Type = std::array<T, Length>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(Length) +
           std::accumulate(value.cbegin(), value.cend(), 0U,
                           [](const std::size_t& sum, const T& element) {
                             return sum + Encoding<T>::Size(element);
                           });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Length, writer);
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
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length)
      return ErrorStatus::InvalidContainerLength;

    for (std::uint64_t i = 0; i < Length; i++) {
      status = Encoding<T>::Read(&(*value)[i], reader);
      if (!status)
        return status;
    }

    return {};
  }
};

template <typename T, std::size_t Length>
struct Encoding<T[Length], EnableIfNotIntegral<T>> : EncodingIO<T[Length]> {
  using Type = T[Length];

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(Length) +
           std::accumulate(std::begin(value), std::end(value), 0U,
                           [](const std::size_t& sum, const T& element) {
                             return sum + Encoding<T>::Size(element);
                           });
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Length, writer);
    if (!status)
      return status;

    for (std::uint64_t i = 0; i < Length; i++) {
      status = Encoding<T>::Write(value[i], writer);
      if (!status)
        return status;
    }

    return {};
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length)
      return ErrorStatus::InvalidContainerLength;

    for (std::uint64_t i = 0; i < Length; i++) {
      status = Encoding<T>::Read(&(*value)[i], reader);
      if (!status)
        return status;
    }

    return {};
  }
};

template <typename T, std::size_t Length>
struct Encoding<std::array<T, Length>, EnableIfIntegral<T>>
    : EncodingIO<std::array<T, Length>> {
  using Type = std::array<T, Length>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = sizeof(T) * Length;
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(size) + size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Length * sizeof(T), writer);
    if (!status)
      return status;

    return writer->WriteRaw(value.begin(), value.end());
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length * sizeof(T))
      return ErrorStatus::InvalidContainerLength;

    return reader->ReadRaw(&(*value)[0], &(*value)[Length]);
  }
};

template <typename T, std::size_t Length>
struct Encoding<T[Length], EnableIfIntegral<T>> : EncodingIO<T[Length]> {
  using Type = T[Length];

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = Length * sizeof(T);
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(size) + size;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Binary;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, const Type& value,
                                   Writer* writer) {
    auto status = Encoding<std::uint64_t>::Write(Length * sizeof(T), writer);
    if (!status)
      return status;

    return writer->WriteRaw(&value[0], &value[Length]);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t size;
    auto status = Encoding<std::uint64_t>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Length * sizeof(T))
      return ErrorStatus::InvalidContainerLength;

    return reader->ReadRaw(&(*value)[0], &(*value)[Length]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_ARRAY_H_
