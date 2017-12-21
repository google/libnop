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

struct UserType {
  enum class Flags {
    Foo = 0b001,
    Bar = 0b010,
    Baz = 0b100,
  };
};
NOP_ENUM_FLAGS(UserType::Flags);

}  // anonymous namespace

TEST(EnumFlags, IsEnumFlags) {
  EXPECT_TRUE(IsEnumFlags<Flags>::value);
  EXPECT_TRUE(IsEnumFlags<UserType::Flags>::value);
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
