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
  BufferReader() = default;
  BufferReader(const BufferReader&) = default;
  BufferReader(const void* buffer, std::size_t size)
      : buffer_{static_cast<const std::uint8_t*>(buffer)}, size_{size} {}

  BufferReader& operator=(const BufferReader&) = default;

  Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus::ReadLimitReached;
    else
      return {};
  }

  Status<void> Read(EncodingByte* prefix) {
    if (index_ < size_) {
      *prefix = static_cast<EncodingByte>(buffer_[index_]);
      index_ += 1;
      return {};
    } else {
      return ErrorStatus::ReadLimitReached;
    }
  }

  Status<void> ReadRaw(void* begin, void* end) {
    using Byte = std::uint8_t;
    Byte* begin_byte = static_cast<Byte*>(begin);
    Byte* end_byte = static_cast<Byte*>(end);

    const std::size_t length_bytes = std::distance(begin_byte, end_byte);
    if (length_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    std::copy(&buffer_[index_], &buffer_[index_ + length_bytes], begin_byte);

    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    index_ += padding_bytes;
    return {};
  }

  bool empty() const { return index_ == size_; }

  std::size_t remaining() const { return size_ - index_; }
  std::size_t capacity() const { return size_; }

 private:
  const std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BUFFER_READER_H_
