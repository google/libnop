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

#ifndef LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_
#define LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_

#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>
#include <nop/types/detail/logical_buffer.h>

//
// Logical buffers support the serialization of structures that contain a pair
// of members, an array and size, that should be logically grouped together to
// behave like a sizeable buffer. This is useful in situations where supporting
// an externally-defined "C" structure with a buffer pattern is needed or where
// dynamic memory allocation is not desirable. Logical buffers are fungible with
// other array-like types, making it easy to substitute an array/size pair when
// needed.
//
// Example of defining a logical buffer in a "C" structure:
//
// // C structure defined in a public header.
// struct SomeCType {
//   char data[256];
//   size_t count;
// };
//
// To handle the externally-defined structure use the macro
// NOP_EXTERNAL_STRUCTURE in the C++ code that handles serializing the data.
// Parenthesis are used to group the pair of members to treat as a logical
// buffer.
//
// NOP_EXTERNAL_STRUCTURE(SomeCType, (data, count));
//
// Example of defining a C++ type with a logical buffer pair:

// template <typename T>
// struct SomeTemplateType {
//  std::array<T, 20> elements;
//  std::size_t count;
//  NOP_STRUCTURE(SomeTemplateType, (elements, count));
// };
//
// Logical buffers are fungible with other array-like types:
//
// struct A {
//  int value;
//  std::vector<int> data;
//  NOP_STRUCTURE(A, value, data);
// };
//
// struct B {
//  int value;
//  int data[256];
//  size_t count;
//  NOP_STRUCTURE(B, value, (data, count));
// };
//
// static_assert(nop::IsFungible<A, B>::value, "!!");
//

namespace nop {

// Encoding type that handles non-integral element types. Logical buffers of
// non-integral element types are encoded the same as non-integral arrays using
// the ARRAY encoding.
template <typename BufferType, typename SizeType, bool IsUnbounded>
struct Encoding<
    LogicalBuffer<BufferType, SizeType, IsUnbounded>,
    EnableIfNotIntegral<typename ArrayTraits<BufferType>::ElementType>>
    : EncodingIO<LogicalBuffer<BufferType, SizeType, IsUnbounded>> {
  using Type = LogicalBuffer<BufferType, SizeType, IsUnbounded>;
  using ValueType = std::remove_const_t<typename Type::ValueType>;
  enum : std::size_t { Length = Type::Length };

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    std::size_t element_size_sum = 0;
    for (const ValueType& element : value)
      element_size_sum += Encoding<ValueType>::Size(element);

    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(value.size()) + element_size_sum;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const SizeType size = static_cast<SizeType>(value.size());
    if (!IsUnbounded && size > Length)
      return ErrorStatus::InvalidContainerLength;

    auto status = Encoding<SizeType>::Write(size, writer);
    if (!status)
      return status;

    for (SizeType i = 0; i < size; i++) {
      status = Encoding<ValueType>::Write(value[i], writer);
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
    else if (!IsUnbounded && size > Length)
      return ErrorStatus::InvalidContainerLength;

    for (SizeType i = 0; i < size; i++) {
      status = Encoding<ValueType>::Read(&(*value)[i], reader);
      if (!status)
        return status;
    }

    value->size() = size;
    return {};
  }
};

// Encoding type that handles integral element types. Logical buffers of
// integral element types are encoded the same as arrays with integral elements
// using the BINARY encoding.
template <typename BufferType, typename SizeType, bool IsUnbounded>
struct Encoding<LogicalBuffer<BufferType, SizeType, IsUnbounded>,
                EnableIfIntegral<typename ArrayTraits<BufferType>::ElementType>>
    : EncodingIO<LogicalBuffer<BufferType, SizeType, IsUnbounded>> {
  using Type = LogicalBuffer<BufferType, SizeType, IsUnbounded>;
  using ValueType = std::remove_const_t<typename Type::ValueType>;
  enum : std::size_t { Length = Type::Length };

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Binary;
  }

  static constexpr std::size_t Size(const Type& value) {
    const std::size_t size = value.size() * sizeof(ValueType);
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
    const SizeType size = value.size();
    if (!IsUnbounded && size > Length)
      return ErrorStatus::InvalidContainerLength;

    auto status = Encoding<SizeType>::Write(size * sizeof(ValueType), writer);
    if (!status)
      return status;

    return writer->Write(value.begin(), value.end());
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size_bytes = 0;
    auto status = Encoding<SizeType>::Read(&size_bytes, reader);
    if (!status) {
      return status;
    } else if ((!IsUnbounded && size_bytes > Length * sizeof(ValueType)) ||
               size_bytes % sizeof(ValueType) != 0) {
      return ErrorStatus::InvalidContainerLength;
    }

    const SizeType size = size_bytes / sizeof(ValueType);
    value->size() = size;
    return reader->Read(value->begin(), value->end());
  }
};

}  // namespace nop

#endif  //  LIBNOP_INCLUDE_NOP_BASE_LOGICAL_BUFFER_H_
