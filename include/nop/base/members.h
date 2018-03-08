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

#ifndef LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_
#define LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_

#include <nop/base/encoding.h>
#include <nop/base/logical_buffer.h>
#include <nop/base/utility.h>
#include <nop/types/detail/member_pointer.h>

namespace nop {

//
// struct/class T encoding format:
//
// +-----+---------+-----//----+
// | STC | INT64:N | N MEMBERS |
// +-----+---------+-----//----+
//
// Members must be valid encodings of their member type.
//

template <typename T>
struct Encoding<T, EnableIfHasMemberList<T>> : EncodingIO<T> {
  static constexpr EncodingByte Prefix(const T& /*value*/) {
    return EncodingByte::Structure;
  }

  static constexpr std::size_t Size(const T& value) {
    return BaseEncodingSize(Prefix(value)) + Encoding<SizeType>::Size(Count) +
           Size(value, Index<Count>{});
  }

  static constexpr bool Match(EncodingByte prefix) {
    return prefix == EncodingByte::Structure;
  }

  template <typename Writer>
  static constexpr Status<void> WritePayload(EncodingByte /*prefix*/,
                                             const T& value, Writer* writer) {
    auto status = Encoding<SizeType>::Write(Count, writer);
    if (!status)
      return status;
    else
      return WriteMembers(value, writer, Index<Count>{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadPayload(EncodingByte /*prefix*/, T* value,
                                            Reader* reader) {
    SizeType size = 0;
    auto status = Encoding<SizeType>::Read(&size, reader);
    if (!status)
      return status;
    else if (size != Count)
      return ErrorStatus::InvalidMemberCount;
    else
      return ReadMembers(value, reader, Index<Count>{});
  }

 private:
  enum : std::size_t { Count = MemberListTraits<T>::MemberList::Count };

  using MemberList = typename MemberListTraits<T>::MemberList;

  template <std::size_t Index>
  using PointerAt = typename MemberList::template At<Index>;

  static constexpr std::size_t Size(const T& /*value*/, Index<0>) { return 0; }

  template <std::size_t index>
  static constexpr std::size_t Size(const T& value, Index<index>) {
    return Size(value, Index<index - 1>{}) + PointerAt<index - 1>::Size(value);
  }

  template <typename Writer>
  static constexpr Status<void> WriteMembers(const T& /*value*/,
                                             Writer* /*writer*/, Index<0>) {
    return {};
  }

  template <std::size_t index, typename Writer>
  static constexpr Status<void> WriteMembers(const T& value, Writer* writer,
                                             Index<index>) {
    auto status = WriteMembers(value, writer, Index<index - 1>{});
    if (!status)
      return status;
    else
      return PointerAt<index - 1>::Write(value, writer, MemberList{});
  }

  template <typename Reader>
  static constexpr Status<void> ReadMembers(T* /*value*/, Reader* /*reader*/,
                                            Index<0>) {
    return {};
  }

  template <std::size_t index, typename Reader>
  static constexpr Status<void> ReadMembers(T* value, Reader* reader,
                                            Index<index>) {
    auto status = ReadMembers(value, reader, Index<index - 1>{});
    if (!status)
      return status;
    else
      return PointerAt<index - 1>::Read(value, reader, MemberList{});
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_BASE_MEMBERS_H_
