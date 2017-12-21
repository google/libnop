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

#include <nop/types/result.h>

using nop::Result;

namespace {

enum class TestError {
  None,  // Required.
  ErrorA,
  ErrorB,
};

}  // anonymous namespace

TEST(Result, Constructor) {
  {
    Result<TestError, int> result;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> result{TestError::ErrorA};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> result{10};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());
  }

  {
    Result<TestError, int> result{Result<TestError, int>{}};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> result{Result<TestError, int>{TestError::ErrorA}};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> result{Result<TestError, int>{10}};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());
  }

  {
    const Result<TestError, int> const_other{};
    Result<TestError, int> result{const_other};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    const Result<TestError, int> const_other{TestError::ErrorA};
    Result<TestError, int> result{const_other};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    const Result<TestError, int> const_other{10};
    Result<TestError, int> result{const_other};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());
  }

  {
    Result<TestError, void> result{};
    EXPECT_FALSE(result.has_error());
  }

  {
    Result<TestError, void> result{TestError::None};
    EXPECT_FALSE(result.has_error());
  }

  // TODO(eieio): Complete tests.
}
