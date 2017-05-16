#include <gtest/gtest.h>

#include <cstdint>

#include <nop/base/utility.h>

using nop::IsIntegral;

TEST(Utility, IsIntegral) {
  EXPECT_TRUE(IsIntegral<std::uint8_t>::value);
  EXPECT_TRUE(IsIntegral<std::int8_t>::value);
  EXPECT_TRUE(IsIntegral<std::uint16_t>::value);
  EXPECT_TRUE(IsIntegral<std::int16_t>::value);
  EXPECT_TRUE(IsIntegral<std::uint32_t>::value);
  EXPECT_TRUE(IsIntegral<std::int32_t>::value);
  EXPECT_TRUE(IsIntegral<std::uint64_t>::value);
  EXPECT_TRUE(IsIntegral<std::int64_t>::value);
  EXPECT_TRUE(IsIntegral<char>::value);
  EXPECT_TRUE(IsIntegral<std::size_t>::value);


  EXPECT_FALSE(IsIntegral<void>::value);
  EXPECT_FALSE(IsIntegral<float>::value);
  EXPECT_FALSE(IsIntegral<double>::value);

  EXPECT_TRUE((IsIntegral<int, short>::value));
  EXPECT_TRUE((IsIntegral<int, short, char>::value));

  EXPECT_FALSE((IsIntegral<float, short, char>::value));
  EXPECT_FALSE((IsIntegral<int, float, char>::value));
  EXPECT_FALSE((IsIntegral<int, short, float>::value));
  EXPECT_FALSE((IsIntegral<int, float, float>::value));
  EXPECT_FALSE((IsIntegral<float, float, char>::value));
  EXPECT_FALSE((IsIntegral<float, float, float>::value));
}
