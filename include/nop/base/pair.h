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

#ifndef LIBNOP_INCLUDE_NOP_BASE_PAIR_H_
#define LIBNOP_INCLUDE_NOP_BASE_PAIR_H_

#include <type_traits>
#include <utility>

#include <nop/base/encoding.h>

namespace nop {

//
// std::pair<T, U> encoding format:
//
// +-----+---------+-------+--------+
// | ARY | INT64:2 | FIRST | SECOND |
// +-----+---------+-------+--------+
//
// First must be a valid encoding of T; second must be a valid encoding of U.
//

template <typename T, typename U>
struct Encoding<std::pair<T, U>> : EncodingIO<std::pair<T, U>> {
  using Type = std::pair<T, U>;

  static constexpr EncodingByte Prefix(const Type& /*value*/) {
    return EncodingByte::Array;
  }

  static constexpr std::size_t Size(const Type& value) {
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(2u) +
           Encoding<First>::Size(value.first) +
           Encoding<Second>::Size(value.second);
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Array;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const Type& value,
                                             Writer* writer) {
    auto status = Encoding<SizeType>::Write(2u, writer);
    if (!status)
      return status;

    status = Encoding<First>::Write(value.first, writer);
    if (!status)
      return status;

    return Encoding<Second>::Write(value.second, writer);
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/,
                                            Type* value, Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != 2u)
      return ErrorStatus::InvalidContainerLength;

    status = Encoding<First>::Read(&value->first, reader);
    if (!status)
      return status;

    return Encoding<Second>::Read(&value->second, reader);
  }

 private:
  using First = std::remove_cv_t<std::remove_reference_t<T>>;
  using Second = std::remove_cv_t<std::remove_reference_t<U>>;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_PAIR_H_
