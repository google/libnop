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

#ifndef LIBNOP_TEST_TEST_WRITER_H_
#define LIBNOP_TEST_TEST_WRITER_H_

#include <cstdint>
#include <iterator>
#include <vector>

#include <nop/base/encoding.h>
#include <nop/base/utility.h>
#include <nop/types/file_handle.h>

namespace nop {

class TestWriter {
 public:
  TestWriter() = default;

  Status<void> Prepare(std::size_t size) {
    data_.reserve(data_.size() + size);
    return {};
  }

  Status<void> Write(std::uint8_t byte) {
    data_.push_back(byte);
    return {};
  }

  Status<void> Write(const void* begin, const void* end) {
    using Byte = std::uint8_t;
    const Byte* begin_byte = static_cast<const Byte*>(begin);
    const Byte* end_byte = static_cast<const Byte*>(end);

    const std::size_t length_bytes = std::distance(begin_byte, end_byte);

    // Extend the buffer to accommodate the data.
    const std::size_t start_offset = data_.size();
    data_.resize(start_offset + length_bytes);

    std::copy(begin_byte, end_byte, &data_[start_offset]);
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes,
                    std::uint8_t padding_value = 0x00) {
    std::vector<std::uint8_t> padding(padding_bytes, padding_value);
    return Write(&*padding.begin(), &*padding.end());
  }

  template <typename HandleType>
  Status<HandleReference> PushHandle(const HandleType& handle) {
    if (handle) {
      HandleReference handle_reference = handles_.size();
      handles_.push_back(handle.get());
      return {handle_reference};
    } else {
      return {kEmptyHandleReference};
    }
  }

  void clear() {
    data_.clear();
    handles_.clear();
  }

  const std::vector<uint8_t>& data() const { return data_; }
  const std::vector<int>& handles() const { return handles_; }

 private:
  std::vector<std::uint8_t> data_;
  std::vector<int> handles_;

  TestWriter(const TestWriter&) = delete;
  void operator=(const TestWriter&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_TEST_TEST_WRITER_H_
