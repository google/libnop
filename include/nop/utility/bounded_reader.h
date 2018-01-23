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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_READER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

// BoundedReader is a reader type that wraps another reader pointer and tracks
// the number of bytes read. Reader operations are transparently passed to the
// underlying reader unless the requested operation would exceed the size limit
// set at construction. BufferReader can also skip padding remaining in the
// input up to the size limit in situations that require specific input payload
// size.
template <typename Reader>
class BoundedReader {
 public:
  BoundedReader() = default;
  BoundedReader(const BoundedReader&) = default;
  BoundedReader(Reader* reader, std::size_t size)
      : reader_{reader}, size_{size} {}

  BoundedReader& operator=(const BoundedReader&) = default;

  Status<void> Ensure(std::size_t size) {
    if (size_ - index_ < size)
      return ErrorStatus::ReadLimitReached;
    else
      return reader_->Ensure(size);
  }

  Status<void> Read(EncodingByte* prefix) {
    if (index_ < size_) {
      auto status = reader_->Read(prefix);
      if (!status)
        return status;

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

    auto status = reader_->ReadRaw(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::ReadLimitReached;

    auto status = reader_->Skip(padding_bytes);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  // Skips any bytes remaining in the limit set at construction.
  Status<void> ReadPadding() {
    const std::size_t padding_bytes = size_ - index_;
    auto status = reader_->Skip(padding_bytes);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  Status<HandleType> GetHandle(HandleReference handle_reference) {
    return reader_->GetHandle(handle_reference);
  }

  bool empty() const { return index_ == size_; }

  std::size_t size() const { return index_; }
  std::size_t capacity() const { return size_; }

 private:
  Reader* reader_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_READER_H_
