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
  BufferWriter() = default;
  BufferWriter(const BufferWriter&) = default;
  BufferWriter(void* buffer, std::size_t size)
      : buffer_{static_cast<std::uint8_t*>(buffer)}, size_{size} {}

  BufferWriter& operator=(const BufferWriter&) = default;

  Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return {};
  }

  Status<void> Write(EncodingByte prefix) {
    if (index_ < size_) {
      buffer_[index_++] = static_cast<std::uint8_t>(prefix);
      return {};
    } else {
      return ErrorStatus::WriteLimitReached;
    }
  }

  Status<void> WriteRaw(const void* begin, const void* end) {
    using Byte = std::uint8_t;
    const Byte* begin_byte = static_cast<const Byte*>(begin);
    const Byte* end_byte = static_cast<const Byte*>(end);

    const std::size_t length_bytes = std::distance(begin_byte, end_byte);
    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    std::copy(begin_byte, end_byte, &buffer_[index_]);

    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes,
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

  std::size_t size() const { return index_; }
  std::size_t capacity() const { return size_; }

 private:
  std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_WRITER_H_
