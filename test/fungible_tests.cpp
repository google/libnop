#include <gtest/gtest.h>

#include <nop/traits/is_fungible.h>
#include <nop/types/variant.h>

using nop::IsFungible;
using nop::Variant;

TEST(FungibleTests, IsFungible) {
  EXPECT_TRUE((IsFungible<int, int>::value));
  EXPECT_TRUE((IsFungible<float, float>::value));
  EXPECT_FALSE((IsFungible<int, float>::value));
  EXPECT_FALSE((IsFungible<float, int>::value));

  EXPECT_TRUE((IsFungible<std::vector<int>, std::vector<int>>::value));
  EXPECT_TRUE((IsFungible<std::vector<float>, std::vector<float>>::value));
  EXPECT_FALSE((IsFungible<std::vector<int>, std::vector<float>>::value));
  EXPECT_FALSE((IsFungible<std::vector<float>, std::vector<int>>::value));

  EXPECT_TRUE((IsFungible<Variant<int>, Variant<int>>::value));
  EXPECT_FALSE((IsFungible<Variant<float>, Variant<int>>::value));

  EXPECT_TRUE((IsFungible<int(int, int), int(int, int)>::value));
  EXPECT_FALSE((IsFungible<int(int, int), float(int, int)>::value));
  EXPECT_FALSE((IsFungible<int(int, int), int(float, int)>::value));
  EXPECT_FALSE((IsFungible<int(int, int), int(int, float)>::value));
}
