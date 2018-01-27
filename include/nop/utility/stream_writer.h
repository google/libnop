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

#ifndef LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_
#define LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_

#include <cstdint>
#include <ostream>

#include <nop/status.h>

namespace nop {

//
// Writer template type that wraps STL output streams.
//
// Implements the basic writer interface on top of an STL output stream type.
//

template <typename OStream>
class StreamWriter {
 public:
  template <typename... Args>
  StreamWriter(Args&&... args) : stream_{std::forward<Args>(args)...} {}
  StreamWriter(const StreamWriter&) = default;
  StreamWriter& operator=(const StreamWriter&) = default;

  Status<void> Prepare(std::size_t /*size*/) { return {}; }

  Status<void> Write(std::uint8_t byte) {
    stream_.put(static_cast<typename OStream::char_type>(byte));
    return ReturnStatus();
  }

  Status<void> Write(const void* begin, const void* end) {
    using CharType = typename OStream::char_type;
    const CharType* begin_char = static_cast<const CharType*>(begin);
    const CharType* end_char = static_cast<const CharType*>(end);

    const std::size_t length_bytes = std::distance(begin_char, end_char);
    stream_.write(begin_char, length_bytes);

    return ReturnStatus();
  }

  Status<void> Skip(std::size_t padding_bytes,
                    std::uint8_t padding_value = 0x00) {
    for (std::size_t i = 0; i < padding_bytes; i++) {
      stream_.put(padding_value);
      auto status = ReturnStatus();
      if (!status)
        return status;
    }

    return {};
  }

  const OStream& stream() const { return stream_; }
  OStream& stream() { return stream_; }
  OStream&& take() { return std::move(stream_); }

 private:
  Status<void> ReturnStatus() {
    if (stream_.bad() || stream_.eof())
      return ErrorStatus::StreamError;
    else
      return {};
  }

  OStream stream_;
};

}  // namespace nop

#endif  // LIBNOP_INLCUDE_NOP_UTILITY_STREAM_WRITER_H_
