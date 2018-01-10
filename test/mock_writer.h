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

#ifndef LIBNOP_TEST_MOCK_WRITER_H_
#define LIBNOP_TEST_MOCK_WRITER_H_

#include <gmock/gmock.h>

namespace nop {
namespace testing {

struct MockWriter {
  MOCK_METHOD1(Prepare, Status<void>(std::size_t size));
  MOCK_METHOD1(Write, Status<void>(EncodingByte prefix));
  MOCK_METHOD2(DoWriteRaw, Status<void>(const void* begin, const void* end));
  MOCK_METHOD2(Skip, Status<void>(std::size_t padding_bytes,
                                  std::uint8_t padding_value));

  template <typename IterBegin, typename IterEnd>
  Status<void> WriteRaw(IterBegin begin, IterEnd end) {
    return DoWriteRaw(&*begin, &*end);
  }
};

}  // namespace testing
}  // namespace nop

#endif  // LIBNOP_TEST_MOCK_WRITER_H_
