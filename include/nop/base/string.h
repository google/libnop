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

#ifndef LIBNOP_INCLUDE_NOP_BASE_STRING_H_
#define LIBNOP_INCLUDE_NOP_BASE_STRING_H_

#include <string>

#include <nop/base/encoding.h>

namespace nop {

//
// std::basic_string<...> encoding format:
//
// +-----+---------+---//----+
// | STR | INT64:N | N BYTES |
// +-----+---------+---//----+
//

template <typename CharType, typename Traits, typename Allocator>
struct Encoding<std::basic_string<CharType, Traits, Allocator>>
    : EncodingIO<std::basic_string<CharType, Traits, Allocator>> {
  using Type = std::basic_string<CharType, Traits, Allocator>;
  enum : std::size_t { CharSize = sizeof(CharType) };

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::String;
  }

  static std::size_t Size(const Type& value) {
    const std::size_t length_bytes = value.length() * CharSize;
    return BaseEncodingSize(Prefix(value)) +
           Encoding<SizeType>::Size(length_bytes) + length_bytes;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::String;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    const std::size_t length = value.length();
    const std::size_t length_bytes = length * CharSize;
    auto status = Encoding<SizeType>::Write(length_bytes, writer);
    if (!status)
      return status;

    return writer->Write(&value[0], &value[length]);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType length_bytes = 0;
    auto status = Encoding<SizeType>::Read(&length_bytes, reader);
    if (!status)
      return status;
    else if (length_bytes % CharSize != 0)
      return ErrorStatus::InvalidStringLength;

    const SizeType size = length_bytes / CharSize;

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous string sizes.
    status = reader->Ensure(size);
    if (!status)
      return status;

    value->resize(size);
    return reader->Read(&(*value)[0], &(*value)[size]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_STRING_H_
