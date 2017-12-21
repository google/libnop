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
#include <type_traits>
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

  Status<void> Read(EncodingByte* prefix) {
    if (index_ < data_.size()) {
      *prefix = static_cast<EncodingByte>(data_[index_++]);
      return {};
    } else {
      return ErrorStatus::ReadLimitReached;
    }
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> ReadRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    if (length_bytes > (data_.size() - index_))
      return ErrorStatus::ReadLimitReached;

    std::copy(&data_[index_], &data_[index_ + length_bytes],
              reinterpret_cast<std::uint8_t*>(&*begin));
    index_ += length_bytes;
    return {};
  }

  Status<void> Skip(std::size_t padding_bytes) {
    std::vector<std::uint8_t> padding(padding_bytes);
    return ReadRaw(padding.begin(), padding.end());
  }

  template <typename HandleType>
  Status<HandleType> GetHandle(HandleReference handle_reference) {
    if (handle_reference < 0)
      return {HandleType{}};
    else if (handle_reference < handles_.size())
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
