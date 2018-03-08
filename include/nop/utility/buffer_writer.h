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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

class BufferWriter {
 public:
  constexpr BufferWriter() = default;
  constexpr BufferWriter(const BufferWriter&) = default;
  constexpr BufferWriter(void* buffer, std::size_t size)
      : buffer_{static_cast<std::uint8_t*>(buffer)}, size_{size} {}

  constexpr BufferWriter& operator=(const BufferWriter&) = default;

  constexpr Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return {};
  }

  constexpr Status<void> Write(std::uint8_t byte) {
    if (index_ < size_) {
      buffer_[index_++] = byte;
      return {};
    } else {
      return ErrorStatus::WriteLimitReached;
    }
  }

  constexpr Status<void> Write(const void* begin, const void* end) {
    using Byte = std::uint8_t;
    const Byte* begin_byte = static_cast<const Byte*>(begin);
    const Byte* end_byte = static_cast<const Byte*>(end);

    const std::size_t length_bytes = end_byte - begin_byte;
    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    for (std::size_t i = 0; i < length_bytes; i++)
      buffer_[index_ + i] = begin_byte[i];

    index_ += length_bytes;
    return {};
  }

  constexpr Status<void> Skip(std::size_t padding_bytes,
                              std::uint8_t padding_value = 0x00) {
    auto status = Prepare(padding_bytes);
    if (!status)
      return status;

    while (padding_bytes) {
      buffer_[index_++] = padding_value;
      padding_bytes--;
    }

    return {};
  }

  constexpr std::size_t size() const { return index_; }
  constexpr std::size_t capacity() const { return size_; }

 private:
  std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
