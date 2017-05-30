#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <nop/base/members.h>
#include <nop/base/utility.h>

using nop::EnableIfHasMemberList;
using nop::EnableIfNotHasMemberList;
using nop::HasMemberList;
using nop::IsIntegral;

namespace {

struct TestWithMemberList {
  int a;
  std::string b;

 private:
  NOP_MEMBERS(TestWithMemberList, a, b);
};

struct TestNoMemberList {
  int a;
  std::string b;
};

template <typename T>
EnableIfHasMemberList<T, bool> CheckHasMemberList(const T& value) {
  return true;
}

template <typename T>
EnableIfNotHasMemberList<T, bool> CheckHasMemberList(const T& value) {
  return false;
}

}  // anonymous namespace

TEST(Utility, HasMemberList) {
  EXPECT_TRUE(HasMemberList<TestWithMemberList>::value);
  EXPECT_FALSE(HasMemberList<TestNoMemberList>::value);

  EXPECT_TRUE(CheckHasMemberList(TestWithMemberList{10, "foo"}));
  EXPECT_FALSE(CheckHasMemberList(TestNoMemberList{10, "foo"}));
}

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
