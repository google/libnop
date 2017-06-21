#include <gtest/gtest.h>

#include <nop/types/enum_flags.h>

using nop::IsEnumFlags;

namespace {

enum class NonFlags {};

enum class Flags {
  None = 0b00,
  A = 0b01,
  B = 0b10,
  C = 0b11,
};
NOP_ENUM_FLAGS(Flags);

}  // anonymous namespace

TEST(EnumFlags, IsEnumFlags) {
  EXPECT_TRUE(IsEnumFlags<Flags>::value);
  EXPECT_FALSE(IsEnumFlags<NonFlags>::value);
}

TEST(EnumFlags, Operators) {
  EXPECT_EQ(Flags::A, Flags::A & Flags::C);
  EXPECT_EQ(Flags::B, Flags::A ^ Flags::C);
  EXPECT_EQ(Flags::C, Flags::A | Flags::B);

  EXPECT_NE(Flags::None, ~Flags::C);
  EXPECT_EQ(Flags::B, Flags::C & ~Flags::A);

  Flags value;

  value = Flags::A;
  value &= Flags::C;
  EXPECT_EQ(Flags::A, value);

  value = Flags::A;
  value ^= Flags::C;
  EXPECT_EQ(Flags::B, value);

  value = Flags::A;
  value |= Flags::B;
  EXPECT_EQ(Flags::C, value);

  EXPECT_FALSE(!!Flags::None);
  EXPECT_TRUE(!!Flags::A);
}
