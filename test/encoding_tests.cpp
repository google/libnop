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

#include <nop/base/encoding.h>

using nop::Encoding;
using nop::EncodingByte;

TEST(Encoding, bool) {
  EXPECT_EQ(EncodingByte::False, Encoding<bool>::Prefix(false));
  EXPECT_EQ(EncodingByte::True, Encoding<bool>::Prefix(true));
  EXPECT_TRUE(Encoding<bool>::Match(EncodingByte::False));
  EXPECT_TRUE(Encoding<bool>::Match(EncodingByte::True));
  EXPECT_FALSE(Encoding<bool>::Match(EncodingByte::U8));
  // TODO(eieio): Test all other values of EncodingByte?
}

TEST(Encoding, char) {
  EXPECT_EQ(EncodingByte::PositiveFixIntMin, Encoding<char>::Prefix(0));
  EXPECT_EQ(EncodingByte::PositiveFixIntMax, Encoding<char>::Prefix(127));
  EXPECT_EQ(EncodingByte::U8, Encoding<char>::Prefix(static_cast<char>(128)));
  EXPECT_EQ(EncodingByte::U8, Encoding<char>::Prefix(static_cast<char>(255)));
  EXPECT_TRUE(Encoding<char>::Match(EncodingByte::PositiveFixIntMin));
  EXPECT_TRUE(Encoding<char>::Match(EncodingByte::PositiveFixIntMax));
  EXPECT_TRUE(Encoding<char>::Match(EncodingByte::U8));
  EXPECT_FALSE(Encoding<char>::Match(EncodingByte::U16));
  // TODO(eieio): Test all other values of EncodingByte?
}

TEST(Encoding, uint8_t) {
  EXPECT_EQ(EncodingByte::PositiveFixIntMin, Encoding<std::uint8_t>::Prefix(0));
  EXPECT_EQ(EncodingByte::PositiveFixIntMax,
            Encoding<std::uint8_t>::Prefix(127));
  EXPECT_EQ(EncodingByte::U8, Encoding<std::uint8_t>::Prefix(128));
  EXPECT_EQ(EncodingByte::U8, Encoding<std::uint8_t>::Prefix(255));
  EXPECT_TRUE(Encoding<std::uint8_t>::Match(EncodingByte::PositiveFixIntMin));
  EXPECT_TRUE(Encoding<std::uint8_t>::Match(EncodingByte::PositiveFixIntMax));
  EXPECT_TRUE(Encoding<std::uint8_t>::Match(EncodingByte::U8));
  EXPECT_FALSE(Encoding<std::uint8_t>::Match(EncodingByte::U16));
  // TODO(eieio): Test all other values of EncodingByte?
}
