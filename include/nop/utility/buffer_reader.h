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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

class BufferReader {
 public:
  constexpr BufferReader() = default;
  constexpr BufferReader(const BufferReader&) = default;
  constexpr BufferReader(const void* buffer, std::size_t size)
      : buffer_{static_cast<const std::uint8_t*>(buffer)}, size_{size} {}

  constexpr BufferReader& operator=(const BufferReader&) = default;

  constexpr Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus::ReadLimitReached;
    else
      return {};
  }

  constexpr Status<void> Read(std::uint8_t* byte) {
    if (index_ < size_) {
      *byte = buffer_[index_];
      index_ += 1;
      return {};
    } else {
      return ErrorStatus::ReadLimitReached;
    }
  }

  constexpr Status<void> Read(void* begin, void* end) {
    using Byte = std::uint8_t;
    Byte* begin_byte = static_cast<Byte*>(begin);
    Byte* end_byte = static_cast<Byte*>(end);

    const std::size_t length_bytes = end_byte - begin_byte;
    if (length_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    for (std::size_t i=0; i < length_bytes; i++)
    	begin_byte[i] = buffer_[index_ + i];

    index_ += length_bytes;
    return {};
  }

  constexpr Status<void> Skip(std::size_t padding_bytes) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    index_ += padding_bytes;
    return {};
  }

  constexpr bool empty() const { return index_ == size_; }

  constexpr std::size_t remaining() const { return size_ - index_; }
  constexpr std::size_t capacity() const { return size_; }

 private:
  const std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
