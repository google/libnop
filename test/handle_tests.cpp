// Copyright 2017 The Native Object Protocols Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <errno.h>
#include <fcntl.h>

#include <array>

#include <nop/types/file_handle.h>
#include <nop/types/handle.h>

using nop::FileHandle;
using nop::UniqueFileHandle;
using nop::UniqueHandle;

namespace {

struct CountingHandlePolicy {
  using Type = int;
  enum : int { kEmptyValue = -1 };

  static int Default() { return kEmptyValue; }
  static void Close(int* value) {
    if (IsValid(*value))
      close_count++;
    *value = kEmptyValue;
  };
  static int Release(int* value) {
    int temp = kEmptyValue;
    std::swap(*value, temp);
    return temp;
  }

  static bool IsValid(int value) { return value != kEmptyValue; }

  static int close_count;
  static void ClearCount() { close_count = 0; }
};

int CountingHandlePolicy::close_count = 0;

}  // anonymous namespace

TEST(Handle, UniqueHandle) {
  using IntHandle = UniqueHandle<CountingHandlePolicy>;

  {
    CountingHandlePolicy::ClearCount();

    IntHandle handle;
    EXPECT_FALSE(handle);
    EXPECT_EQ(0, CountingHandlePolicy::close_count);
  }

  {
    CountingHandlePolicy::ClearCount();

    IntHandle handle{10};
    EXPECT_TRUE(handle);
    EXPECT_EQ(0, CountingHandlePolicy::close_count);
    handle.close();
    EXPECT_EQ(1, CountingHandlePolicy::close_count);
  }
  EXPECT_EQ(1, CountingHandlePolicy::close_count);

  {
    CountingHandlePolicy::ClearCount();

    IntHandle handle{10};
    EXPECT_TRUE(handle);
    EXPECT_EQ(10, handle.get());
    EXPECT_EQ(0, CountingHandlePolicy::close_count);
  }
  EXPECT_EQ(1, CountingHandlePolicy::close_count);

  {
    CountingHandlePolicy::ClearCount();

    IntHandle handle{10};
    IntHandle handle2 = std::move(handle);
    EXPECT_FALSE(handle);
    EXPECT_EQ(CountingHandlePolicy::kEmptyValue, handle.get());
    EXPECT_TRUE(handle2);
    EXPECT_EQ(10, handle2.get());
    EXPECT_EQ(0, CountingHandlePolicy::close_count);
  }
  EXPECT_EQ(1, CountingHandlePolicy::close_count);

  {
    CountingHandlePolicy::ClearCount();

    IntHandle handle{10};
    IntHandle handle2{std::move(handle)};
    EXPECT_FALSE(handle);
    EXPECT_TRUE(handle2);
    EXPECT_EQ(0, CountingHandlePolicy::close_count);
  }
  EXPECT_EQ(1, CountingHandlePolicy::close_count);
}

TEST(Handle, FileHandle) {
  int fd = open("/dev/zero", O_RDONLY);
  UniqueFileHandle file_handle{fd};
  ASSERT_TRUE(file_handle);
  EXPECT_EQ(fd, file_handle.get());

  UniqueFileHandle file_handle2 = std::move(file_handle);
  EXPECT_FALSE(file_handle);
  EXPECT_TRUE(file_handle2);

  std::array<char, 10> buffer;
  buffer.fill('\0');

  EXPECT_EQ(static_cast<ssize_t>(buffer.size()),
            read(file_handle2.get(), buffer.data(), buffer.size()));

  {
    FileHandle handle{file_handle2};
    EXPECT_EQ(static_cast<ssize_t>(buffer.size()),
              read(handle.get(), buffer.data(), buffer.size()));
  }
  EXPECT_EQ(static_cast<ssize_t>(buffer.size()),
            read(file_handle2.get(), buffer.data(), buffer.size()));

  {
    UniqueFileHandle file_handle3{std::move(file_handle2)};
    EXPECT_FALSE(file_handle2);
    EXPECT_TRUE(file_handle3);
  }
  EXPECT_EQ(-1, read(fd, buffer.data(), buffer.size()));
  EXPECT_EQ(EBADF, errno);
}
