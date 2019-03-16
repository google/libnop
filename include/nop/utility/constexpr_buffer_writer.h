/*
 * Copyright 2019 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_CONSTEXPR_BUFFER_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_CONSTEXPR_BUFFER_WRITER_H_

#include <cstddef>
#include <cstdint>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>

namespace nop {

// A writer type that supports constexpr serialization into a byte buffer. This
// type is sub-optimal for non-constexpr serialization: use BufferWriter or
// PedanticBufferWriter for runtime serialization into a byte buffer.
class ConstexprBufferWriter {
 public:
  constexpr ConstexprBufferWriter() = default;
  constexpr ConstexprBufferWriter(const ConstexprBufferWriter&) = default;
  template <std::size_t Size>
  constexpr ConstexprBufferWriter(std::uint8_t (&buffer)[Size])
      : buffer_{buffer}, size_{Size} {}
  constexpr ConstexprBufferWriter(std::uint8_t* buffer, std::size_t size)
      : buffer_{buffer}, size_{size} {}

  constexpr ConstexprBufferWriter& operator=(const ConstexprBufferWriter&) =
      default;

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

  template <typename T, typename Enable = EnableIfArithmetic<T>>
  constexpr Status<void> Write(const T* begin, const T* end) {
    const std::size_t element_size = sizeof(T);
    const std::size_t length = end - begin;
    const std::size_t length_bytes = length * element_size;

    if (length_bytes > (size_ - index_))
      return ErrorStatus::WriteLimitReached;

    for (std::size_t i = 0; i < length; i++)
      WriteElement(begin[i], i * element_size);

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
  // Write an integer element to the byte buffer with constexpr-compatible
  // conversions from larger integer types.
  constexpr void WriteElement(std::uint8_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
  }
  constexpr void WriteElement(std::int8_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint8_t>(value), offset);
  }
  constexpr void WriteElement(char value, std::size_t offset) {
    WriteElement(static_cast<std::uint8_t>(value), offset);
  }
  constexpr void WriteElement(std::uint16_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
    buffer_[index_ + offset + 1] = value >> 8;
  }
  constexpr void WriteElement(std::int16_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint16_t>(value), offset);
  }
  constexpr void WriteElement(std::uint32_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
    buffer_[index_ + offset + 1] = value >> 8;
    buffer_[index_ + offset + 2] = value >> 16;
    buffer_[index_ + offset + 3] = value >> 24;
  }
  constexpr void WriteElement(std::int32_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint32_t>(value), offset);
  }
  constexpr void WriteElement(std::uint64_t value, std::size_t offset) {
    buffer_[index_ + offset + 0] = value >> 0;
    buffer_[index_ + offset + 1] = value >> 8;
    buffer_[index_ + offset + 2] = value >> 16;
    buffer_[index_ + offset + 3] = value >> 24;
    buffer_[index_ + offset + 4] = value >> 32;
    buffer_[index_ + offset + 5] = value >> 40;
    buffer_[index_ + offset + 6] = value >> 48;
    buffer_[index_ + offset + 7] = value >> 56;
  }
  constexpr void WriteElement(std::int64_t value, std::size_t offset) {
    WriteElement(static_cast<std::uint64_t>(value), offset);
  }

  // TODO(eieio): At the time of this writing there isn't simple way to get the
  // raw bytes of a floating point type in a constexpr expression.

  std::uint8_t* buffer_{nullptr};
  std::size_t size_{0};
  std::size_t index_{0};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_CONSTEXPR_BUFFER_WRITER_H_
