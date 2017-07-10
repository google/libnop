#include <gtest/gtest.h>

#include <string>

#include <nop/types/optional.h>

using nop::Optional;

TEST(Optional, Basic) {
  {
    Optional<int> value;
    EXPECT_TRUE(value.empty());
    EXPECT_FALSE(value);

    value = 10;
    EXPECT_FALSE(value.empty());
    EXPECT_TRUE(value);
    EXPECT_EQ(10, value.get());

    value.clear();
    EXPECT_TRUE(value.empty());
    EXPECT_FALSE(value);
  }

  {
    Optional<int> value{20};
    EXPECT_FALSE(value.empty());
    EXPECT_TRUE(value);

    value = 10;
    EXPECT_FALSE(value.empty());
    EXPECT_TRUE(value);
    EXPECT_EQ(10, value.get());

    value = Optional<int>{30};
    EXPECT_FALSE(value.empty());
    EXPECT_TRUE(value);

    value = Optional<int>{};
    EXPECT_TRUE(value.empty());
    EXPECT_FALSE(value);
  }

  // TODO(eieio): Thorough constructor/destructor tests.
}
