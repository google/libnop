/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_TEST_MOCK_READER_H_
#define LIBNOP_TEST_MOCK_READER_H_

#include <cstdint>
#include <gmock/gmock.h>

#include <nop/status.h>
#include <nop/types/handle.h>

namespace nop {
namespace testing {

struct MockReader {
  MOCK_METHOD1(Ensure, Status<void>(std::size_t size));
  MOCK_METHOD1(Read, Status<void>(std::uint8_t* byte));
  MOCK_METHOD2(Read, Status<void>(void* begin, void* end));
  MOCK_METHOD1(Skip, Status<void>(std::size_t padding_bytes));
  MOCK_METHOD1(GetIntHandle, Status<int>(HandleReference handle_reference));

  template <typename HandleType>
  Status<HandleType> GetHandle(HandleReference handle_reference) {
    auto status = GetIntHandle(handle_reference);
    if (!status)
      return status.error();
    else
      return HandleType{status.get()};
  }
};

}  // namespace testing
}  // namespace nop

#endif  // LIBNOP_TEST_MOCK_READER_H_
