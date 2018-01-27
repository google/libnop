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

#ifndef LIBNOP_TEST_TEST_READER_H_
#define LIBNOP_TEST_TEST_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

class TestReader {
 public:
  TestReader() = default;

  Status<void> Ensure(std::size_t size) {
    if (data_.size() - index_ < size)
      return ErrorStatus::ReadLimitReached;
    else
      return {};
  }

  Status<void> Read(std::uint8_t* byte) {
    if (index_ < data_.size()) {
      *byte = data_[index_++];
      return {};
    } else {
      return ErrorStatus::ReadLimitReached;
    }
  }

  Status<void> Read(void* begin, void* end) {
    using Byte = std::uint8_t;
    Byte* begin_byte = static_cast<Byte*>(begin);
    Byte* end_byte = static_cast<Byte*>(end);

    const std::size_t length_bytes = std::distance(begin_byte, end_byte);
    if (length_bytes > (data_.size() - index_))
      return ErrorStatus::ReadLimitReached;

    std::copy(&data_[index_], &data_[index_ + length_bytes], begin_byte);
    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes) {
    std::vector<std::uint8_t> padding(padding_bytes);
    return Read(&*padding.begin(), &*padding.end());
  }

  template <typename HandleType>
  Status<HandleType> GetHandle(HandleReference handle_reference) {
    if (handle_reference < 0)
      return {HandleType{}};
    else if (handle_reference < static_cast<HandleReference>(handles_.size()))
      return {HandleType{handles_[handle_reference]}};
    else
      return ErrorStatus::InvalidHandleReference;
  }

  void Set(std::vector<std::uint8_t> data) {
    data_ = std::move(data);
    index_ = 0;
  }

  void SetHandles(std::vector<int> handles) { handles_ = std::move(handles); }

 private:
  std::vector<std::uint8_t> data_;
  std::vector<int> handles_;
  std::size_t index_{0};

  TestReader(const TestReader&) = delete;
  void operator=(const TestReader&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_TEST_TEST_READER_H_
