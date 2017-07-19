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
    EXPECT_TRUE(result.has_error());
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
    EXPECT_TRUE(result.has_error());
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
    EXPECT_TRUE(result.has_error());
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

  // TODO(eieio): Complete tests.
}
