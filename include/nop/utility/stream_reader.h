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

#ifndef LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_
#define LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_

#include <cstdint>
#include <istream>

#include <nop/status.h>

namespace nop {

//
// Reader template type that wraps STL input streams.
//
// Implements the basic reader interface on top of an STL input stream type.
//

template <typename IStream>
class StreamReader {
 public:
  template <typename... Args>
  StreamReader(Args&&... args) : stream_{std::forward<Args>(args)...} {}
  StreamReader(const StreamReader&) = default;
  StreamReader& operator=(const StreamReader&) = default;

  Status<void> Ensure(std::size_t /*size*/) { return {}; }

  Status<void> Read(std::uint8_t* byte) {
    using CharType = typename IStream::char_type;
    stream_.read(reinterpret_cast<CharType*>(byte), sizeof(std::uint8_t));

    return ReturnStatus();
  }

  Status<void> Read(void* begin, void* end) {
    using CharType = typename IStream::char_type;
    CharType* begin_char = static_cast<CharType*>(begin);
    CharType* end_char = static_cast<CharType*>(end);

    const std::size_t length_bytes = std::distance(begin_char, end_char);
    stream_.read(begin_char, length_bytes);

    return ReturnStatus();
  }

  Status<void> Skip(std::size_t padding_bytes) {
    stream_.seekg(padding_bytes, std::ios_base::cur);
    return ReturnStatus();
  }

  const IStream& stream() const { return stream_; }
  IStream& stream() { return stream_; }
  IStream&& take() { return std::move(stream_); }

 private:
  Status<void> ReturnStatus() {
    if (stream_.bad() || stream_.eof())
      return ErrorStatus::StreamError;
    else
      return {};
  }

  IStream stream_;
};

}  // namespace nop

#endif  // LIBNOP_INLCUDE_NOP_UTILITY_STREAM_READER_H_
