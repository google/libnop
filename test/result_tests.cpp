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

Result<TestError, int> GetResult(TestError error) { return error; }

Result<TestError, int> GetResult(int value) { return value; }

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
    const Result<TestError, int> result;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> result{TestError::None};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    const Result<TestError, int> result{TestError::None};
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
    const Result<TestError, int> result{TestError::ErrorA};
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
    const Result<TestError, int> result{10};
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
    const Result<TestError, int> result{Result<TestError, int>{}};
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
    const Result<TestError, int> result{
        Result<TestError, int>{TestError::ErrorA}};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> other{TestError::ErrorA};
    Result<TestError, int> result{std::move(other)};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
    EXPECT_FALSE(other.has_error());
    EXPECT_EQ(TestError::None, other.error());
    EXPECT_FALSE(other.has_value());
    EXPECT_EQ(nullptr, &other.get());
  }

  {
    Result<TestError, int> other{TestError::ErrorA};
    const Result<TestError, int> result{std::move(other)};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
    EXPECT_FALSE(other.has_error());
    EXPECT_EQ(TestError::None, other.error());
    EXPECT_FALSE(other.has_value());
    EXPECT_EQ(nullptr, &other.get());
  }

  {
    Result<TestError, int> result{GetResult(TestError::ErrorA)};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    const Result<TestError, int> result{GetResult(TestError::ErrorA)};
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
    const Result<TestError, int> result{Result<TestError, int>{10}};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());
  }

  {
    Result<TestError, int> result{GetResult(10)};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());
  }

  {
    const Result<TestError, int> result{GetResult(10)};
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
    EXPECT_EQ(TestError::None, result.error());
  }

  {
    Result<TestError, void> result{TestError::None};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
  }

  {
    Result<TestError, void> result{TestError::ErrorA};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
  }

  {
    Result<TestError, void> result{Result<TestError, void>{}};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
  }

  {
    Result<TestError, void> result{Result<TestError, void>{TestError::ErrorA}};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
  }

  {
    Result<TestError, void> other{TestError::ErrorA};
    Result<TestError, void> result{std::move(other)};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(other.has_error());
    EXPECT_EQ(TestError::None, other.error());
  }

  {
    const Result<TestError, void> other{TestError::ErrorA};
    Result<TestError, void> result{other};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_TRUE(other.has_error());
    EXPECT_EQ(TestError::ErrorA, other.error());
  }
}

TEST(Result, Assignment) {
  {
    Result<TestError, int> result = TestError::None;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());

    result = TestError::ErrorA;
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());

    result = 10;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());
  }

  {
    Result<TestError, int> result = 10;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());

    result = 20;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(20, result.get());

    const int const_value = 30;
    result = const_value;
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(30, result.get());

    result = TestError::ErrorA;
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, int> result{10};
    Result<TestError, int> error{TestError::ErrorA};
    result = error;
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());

    EXPECT_TRUE(error.has_error());
    EXPECT_EQ(TestError::ErrorA, error.error());
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(nullptr, &error.get());
  }

  {
    Result<TestError, int> result{10};
    Result<TestError, int> error{TestError::ErrorA};
    result = std::move(error);
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());

    EXPECT_FALSE(error.has_error());
    EXPECT_EQ(TestError::None, error.error());
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(nullptr, &error.get());
  }
}

TEST(Result, Clear) {
  {
    Result<TestError, int> result{10};
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(nullptr, &result.get());
    EXPECT_EQ(10, result.get());

    result.clear();
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

    result.clear();
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(nullptr, &result.get());
  }

  {
    Result<TestError, void> result{TestError::ErrorA};
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(TestError::ErrorA, result.error());

    result.clear();
    EXPECT_FALSE(result.has_error());
    EXPECT_EQ(TestError::None, result.error());
  }
}
