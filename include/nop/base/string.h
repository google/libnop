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
// std::string encoding format:
//
// +-----+---------+---//----+
// | STR | INT64:N | N BYTES |
// +-----+---------+---//----+
//

template <>
struct Encoding<std::string> : EncodingIO<std::string> {
  using Type = std::string;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::String;
  }

  static std::size_t Size(const Type& value) {
    const std::size_t length_bytes = value.length() * sizeof(Type::value_type);
    return BaseEncodingSize(Prefix(value)) +
           Encoding<std::uint64_t>::Size(length_bytes) + length_bytes;
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::String;
  }

  template <typename Writer>
  static Status<void> WritePayload(EncodingByte /*prefix*/, const Type& value,
                                   Writer* writer) {
    const std::size_t length = value.length();
    const std::size_t length_bytes = length * sizeof(Type::value_type);
    auto status = Encoding<std::uint64_t>::Write(length_bytes, writer);
    if (!status)
      return status;

    return writer->WriteRaw(&value[0], &value[length]);
  }

  template <typename Reader>
  static Status<void> ReadPayload(EncodingByte /*prefix*/, Type* value,
                                  Reader* reader) {
    std::uint64_t length_bytes;
    auto status = Encoding<std::uint64_t>::Read(&length_bytes, reader);
    if (!status)
      return status;
    else if (length_bytes % sizeof(Type::value_type) != 0)
      return ErrorStatus::InvalidStringLength;

    const std::uint64_t size = length_bytes / sizeof(Type::value_type);

    // Make sure the reader has enough data to fulfill the requested size as a
    // defense against abusive or erroneous string sizes.
    status = reader->Ensure(size);
    if (!status)
      return status;

    value->resize(size);
    return reader->ReadRaw(&(*value)[0], &(*value)[size]);
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_STRING_H_
