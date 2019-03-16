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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

// BoundedWriter is a writer type that wraps another writer pointer and tracks
// the number of bytes written. Writer operations are transparently passed to
// the underlying writer unless the requested operation would exceed the size
// limit set at construction. BufferWriter can also pad the output up to the
// size limit in situations that require specific output payload size.
template <typename Writer>
class BoundedWriter {
 public:
  constexpr BoundedWriter() = default;
  constexpr BoundedWriter(const BoundedWriter&) = default;
  constexpr BoundedWriter(Writer* writer, std::size_t size)
      : writer_{writer}, size_{size} {}

  constexpr BoundedWriter& operator=(const BoundedWriter&) = default;

  constexpr Status<void> Prepare(std::size_t size) {
    if (index_ + size > size_)
      return ErrorStatus::WriteLimitReached;
    else
      return writer_->Prepare(size);
  }

  constexpr Status<void> Write(std::uint8_t byte) {
    if (index_ < size_) {
      auto status = writer_->Write(byte);
      if (!status)
        return status;

      index_ += 1;
      return {};
    } else {
      return ErrorStatus::WriteLimitReached;
    }
  }

  template <typename T, typename Enabel = EnableIfArithmetic<T>>
  constexpr Status<void> Write(const T* begin, const T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    auto status = writer_->Write(begin, end);
    if (!status)
      return status;

    index_ += length_bytes;
    return {};
  }

  constexpr Status<void> Skip(std::size_t padding_bytes,
                              std::uint8_t padding_value = 0x00) {
    if (padding_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    auto status = writer_->Skip(padding_bytes, padding_value);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  // Fills out any remaining bytes with the given padding value.
  constexpr Status<void> WritePadding(std::uint8_t padding_value = 0x00) {
    const std::size_t padding_bytes = size_ - index_;
    auto status = writer_->Skip(padding_bytes, padding_value);
    if (!status)
      return status;

    index_ += padding_bytes;
    return {};
  }

  template <typename HandleType>
  constexpr Status<HandleType> PushHandle(const HandleType& handle) {
    return writer_->PushHandle(handle);
  }

  constexpr std::size_t size() const { return index_; }
  constexpr std::size_t capacity() const { return size_; }

 private:
  Writer* writer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BOUNDED_WRITER_H_
