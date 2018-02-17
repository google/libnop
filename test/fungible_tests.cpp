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

#include <array>

#include <nop/base/logical_buffer.h>
#include <nop/structure.h>
#include <nop/traits/is_fungible.h>
#include <nop/types/optional.h>
#include <nop/types/result.h>
#include <nop/types/variant.h>
#include <nop/value.h>

using nop::Entry;
using nop::IsFungible;
using nop::LogicalBuffer;
using nop::Optional;
using nop::Result;
using nop::Variant;

// Test fungibility of basic arithmetic types.
// TODO(eieio): Figure out a less verbose way to do this.
TEST(FungibleTests, Arithmetic) {
  EXPECT_TRUE((IsFungible<char, char>::value));
  EXPECT_FALSE((IsFungible<char, signed char>::value));
  EXPECT_FALSE((IsFungible<char, unsigned char>::value));
  EXPECT_FALSE((IsFungible<char, int>::value));
  EXPECT_FALSE((IsFungible<char, unsigned int>::value));
  EXPECT_FALSE((IsFungible<char, long>::value));
  EXPECT_FALSE((IsFungible<char, unsigned long>::value));
  EXPECT_FALSE((IsFungible<char, long long>::value));
  EXPECT_FALSE((IsFungible<char, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<char, short>::value));
  EXPECT_FALSE((IsFungible<char, unsigned short>::value));
  EXPECT_FALSE((IsFungible<char, float>::value));
  EXPECT_FALSE((IsFungible<char, double>::value));
  EXPECT_FALSE((IsFungible<signed char, char>::value));
  EXPECT_TRUE((IsFungible<signed char, signed char>::value));
  EXPECT_FALSE((IsFungible<signed char, unsigned char>::value));
  EXPECT_FALSE((IsFungible<signed char, int>::value));
  EXPECT_FALSE((IsFungible<signed char, unsigned int>::value));
  EXPECT_FALSE((IsFungible<signed char, long>::value));
  EXPECT_FALSE((IsFungible<signed char, unsigned long>::value));
  EXPECT_FALSE((IsFungible<signed char, long long>::value));
  EXPECT_FALSE((IsFungible<signed char, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<signed char, short>::value));
  EXPECT_FALSE((IsFungible<signed char, unsigned short>::value));
  EXPECT_FALSE((IsFungible<signed char, float>::value));
  EXPECT_FALSE((IsFungible<signed char, double>::value));
  EXPECT_FALSE((IsFungible<unsigned char, char>::value));
  EXPECT_FALSE((IsFungible<unsigned char, signed char>::value));
  EXPECT_TRUE((IsFungible<unsigned char, unsigned char>::value));
  EXPECT_FALSE((IsFungible<unsigned char, int>::value));
  EXPECT_FALSE((IsFungible<unsigned char, unsigned int>::value));
  EXPECT_FALSE((IsFungible<unsigned char, long>::value));
  EXPECT_FALSE((IsFungible<unsigned char, unsigned long>::value));
  EXPECT_FALSE((IsFungible<unsigned char, long long>::value));
  EXPECT_FALSE((IsFungible<unsigned char, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<unsigned char, short>::value));
  EXPECT_FALSE((IsFungible<unsigned char, unsigned short>::value));
  EXPECT_FALSE((IsFungible<unsigned char, float>::value));
  EXPECT_FALSE((IsFungible<unsigned char, double>::value));
  EXPECT_FALSE((IsFungible<int, char>::value));
  EXPECT_FALSE((IsFungible<int, signed char>::value));
  EXPECT_FALSE((IsFungible<int, unsigned char>::value));
  EXPECT_TRUE((IsFungible<int, int>::value));
  EXPECT_FALSE((IsFungible<int, unsigned int>::value));
  EXPECT_FALSE((IsFungible<int, long>::value));
  EXPECT_FALSE((IsFungible<int, unsigned long>::value));
  EXPECT_FALSE((IsFungible<int, long long>::value));
  EXPECT_FALSE((IsFungible<int, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<int, short>::value));
  EXPECT_FALSE((IsFungible<int, unsigned short>::value));
  EXPECT_FALSE((IsFungible<int, float>::value));
  EXPECT_FALSE((IsFungible<int, double>::value));
  EXPECT_FALSE((IsFungible<unsigned int, char>::value));
  EXPECT_FALSE((IsFungible<unsigned int, signed char>::value));
  EXPECT_FALSE((IsFungible<unsigned int, unsigned char>::value));
  EXPECT_FALSE((IsFungible<unsigned int, int>::value));
  EXPECT_TRUE((IsFungible<unsigned int, unsigned int>::value));
  EXPECT_FALSE((IsFungible<unsigned int, long>::value));
  EXPECT_FALSE((IsFungible<unsigned int, unsigned long>::value));
  EXPECT_FALSE((IsFungible<unsigned int, long long>::value));
  EXPECT_FALSE((IsFungible<unsigned int, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<unsigned int, short>::value));
  EXPECT_FALSE((IsFungible<unsigned int, unsigned short>::value));
  EXPECT_FALSE((IsFungible<unsigned int, float>::value));
  EXPECT_FALSE((IsFungible<unsigned int, double>::value));
  EXPECT_FALSE((IsFungible<long, char>::value));
  EXPECT_FALSE((IsFungible<long, signed char>::value));
  EXPECT_FALSE((IsFungible<long, unsigned char>::value));
  EXPECT_FALSE((IsFungible<long, int>::value));
  EXPECT_FALSE((IsFungible<long, unsigned int>::value));
  EXPECT_TRUE((IsFungible<long, long>::value));
  EXPECT_FALSE((IsFungible<long, unsigned long>::value));
  EXPECT_FALSE((IsFungible<long, long long>::value));
  EXPECT_FALSE((IsFungible<long, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<long, short>::value));
  EXPECT_FALSE((IsFungible<long, unsigned short>::value));
  EXPECT_FALSE((IsFungible<long, float>::value));
  EXPECT_FALSE((IsFungible<long, double>::value));
  EXPECT_FALSE((IsFungible<unsigned long, char>::value));
  EXPECT_FALSE((IsFungible<unsigned long, signed char>::value));
  EXPECT_FALSE((IsFungible<unsigned long, unsigned char>::value));
  EXPECT_FALSE((IsFungible<unsigned long, int>::value));
  EXPECT_FALSE((IsFungible<unsigned long, unsigned int>::value));
  EXPECT_FALSE((IsFungible<unsigned long, long>::value));
  EXPECT_TRUE((IsFungible<unsigned long, unsigned long>::value));
  EXPECT_FALSE((IsFungible<unsigned long, long long>::value));
  EXPECT_FALSE((IsFungible<unsigned long, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<unsigned long, short>::value));
  EXPECT_FALSE((IsFungible<unsigned long, unsigned short>::value));
  EXPECT_FALSE((IsFungible<unsigned long, float>::value));
  EXPECT_FALSE((IsFungible<unsigned long, double>::value));
  EXPECT_FALSE((IsFungible<long long, char>::value));
  EXPECT_FALSE((IsFungible<long long, signed char>::value));
  EXPECT_FALSE((IsFungible<long long, unsigned char>::value));
  EXPECT_FALSE((IsFungible<long long, int>::value));
  EXPECT_FALSE((IsFungible<long long, unsigned int>::value));
  EXPECT_FALSE((IsFungible<long long, long>::value));
  EXPECT_FALSE((IsFungible<long long, unsigned long>::value));
  EXPECT_TRUE((IsFungible<long long, long long>::value));
  EXPECT_FALSE((IsFungible<long long, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<long long, short>::value));
  EXPECT_FALSE((IsFungible<long long, unsigned short>::value));
  EXPECT_FALSE((IsFungible<long long, float>::value));
  EXPECT_FALSE((IsFungible<long long, double>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, char>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, signed char>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, unsigned char>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, int>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, unsigned int>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, long>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, unsigned long>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, long long>::value));
  EXPECT_TRUE((IsFungible<unsigned long long, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, short>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, unsigned short>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, float>::value));
  EXPECT_FALSE((IsFungible<unsigned long long, double>::value));
  EXPECT_FALSE((IsFungible<short, char>::value));
  EXPECT_FALSE((IsFungible<short, signed char>::value));
  EXPECT_FALSE((IsFungible<short, unsigned char>::value));
  EXPECT_FALSE((IsFungible<short, int>::value));
  EXPECT_FALSE((IsFungible<short, unsigned int>::value));
  EXPECT_FALSE((IsFungible<short, long>::value));
  EXPECT_FALSE((IsFungible<short, unsigned long>::value));
  EXPECT_FALSE((IsFungible<short, long long>::value));
  EXPECT_FALSE((IsFungible<short, unsigned long long>::value));
  EXPECT_TRUE((IsFungible<short, short>::value));
  EXPECT_FALSE((IsFungible<short, unsigned short>::value));
  EXPECT_FALSE((IsFungible<short, float>::value));
  EXPECT_FALSE((IsFungible<short, double>::value));
  EXPECT_FALSE((IsFungible<unsigned short, char>::value));
  EXPECT_FALSE((IsFungible<unsigned short, signed char>::value));
  EXPECT_FALSE((IsFungible<unsigned short, unsigned char>::value));
  EXPECT_FALSE((IsFungible<unsigned short, int>::value));
  EXPECT_FALSE((IsFungible<unsigned short, unsigned int>::value));
  EXPECT_FALSE((IsFungible<unsigned short, long>::value));
  EXPECT_FALSE((IsFungible<unsigned short, unsigned long>::value));
  EXPECT_FALSE((IsFungible<unsigned short, long long>::value));
  EXPECT_FALSE((IsFungible<unsigned short, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<unsigned short, short>::value));
  EXPECT_TRUE((IsFungible<unsigned short, unsigned short>::value));
  EXPECT_FALSE((IsFungible<unsigned short, float>::value));
  EXPECT_FALSE((IsFungible<unsigned short, double>::value));
  EXPECT_FALSE((IsFungible<float, char>::value));
  EXPECT_FALSE((IsFungible<float, signed char>::value));
  EXPECT_FALSE((IsFungible<float, unsigned char>::value));
  EXPECT_FALSE((IsFungible<float, int>::value));
  EXPECT_FALSE((IsFungible<float, unsigned int>::value));
  EXPECT_FALSE((IsFungible<float, long>::value));
  EXPECT_FALSE((IsFungible<float, unsigned long>::value));
  EXPECT_FALSE((IsFungible<float, long long>::value));
  EXPECT_FALSE((IsFungible<float, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<float, short>::value));
  EXPECT_FALSE((IsFungible<float, unsigned short>::value));
  EXPECT_TRUE((IsFungible<float, float>::value));
  EXPECT_FALSE((IsFungible<float, double>::value));
  EXPECT_FALSE((IsFungible<double, char>::value));
  EXPECT_FALSE((IsFungible<double, signed char>::value));
  EXPECT_FALSE((IsFungible<double, unsigned char>::value));
  EXPECT_FALSE((IsFungible<double, int>::value));
  EXPECT_FALSE((IsFungible<double, unsigned int>::value));
  EXPECT_FALSE((IsFungible<double, long>::value));
  EXPECT_FALSE((IsFungible<double, unsigned long>::value));
  EXPECT_FALSE((IsFungible<double, long long>::value));
  EXPECT_FALSE((IsFungible<double, unsigned long long>::value));
  EXPECT_FALSE((IsFungible<double, short>::value));
  EXPECT_FALSE((IsFungible<double, unsigned short>::value));
  EXPECT_FALSE((IsFungible<double, float>::value));
  EXPECT_TRUE((IsFungible<double, double>::value));
}

// Test fungibility of std::array, std::vector, and C-style arrays.
TEST(FungibleTests, ArrayVector) {
  using IntArray1 = std::array<int, 1>;
  using IntArray2 = std::array<int, 2>;
  using FloatArray1 = std::array<float, 1>;
  using FloatArray2 = std::array<float, 2>;

  // Test combinations of T={int,float} and Size={1,2}.
  EXPECT_TRUE((IsFungible<IntArray1, IntArray1>::value));
  EXPECT_TRUE((IsFungible<FloatArray1, FloatArray1>::value));
  EXPECT_FALSE((IsFungible<IntArray1, FloatArray1>::value));
  EXPECT_FALSE((IsFungible<IntArray1, IntArray2>::value));
  EXPECT_FALSE((IsFungible<IntArray2, FloatArray2>::value));

  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  // Test combinations of T={int,float}.
  EXPECT_TRUE((IsFungible<IntVector, IntVector>::value));
  EXPECT_TRUE((IsFungible<FloatVector, FloatVector>::value));
  EXPECT_FALSE((IsFungible<IntVector, FloatVector>::value));
  EXPECT_FALSE((IsFungible<FloatVector, IntVector>::value));

  // Test combinations of std::array and std::vector.
  EXPECT_TRUE((IsFungible<IntVector, IntArray1>::value));
  EXPECT_TRUE((IsFungible<IntVector, IntArray2>::value));
  EXPECT_FALSE((IsFungible<IntVector, FloatArray1>::value));
  EXPECT_FALSE((IsFungible<IntVector, FloatArray2>::value));

  using IntCArray1 = int[1];
  using IntCArray2 = int[2];
  using FloatCArray1 = float[1];
  using FloatCArray2 = float[2];

  // Test combinations of T={int,float} and Size={1,2}.
  EXPECT_TRUE((IsFungible<IntCArray1, IntCArray1>::value));
  EXPECT_TRUE((IsFungible<FloatCArray1, FloatCArray1>::value));
  EXPECT_FALSE((IsFungible<IntCArray1, FloatCArray1>::value));
  EXPECT_FALSE((IsFungible<IntCArray1, IntCArray2>::value));
  EXPECT_FALSE((IsFungible<IntCArray2, FloatCArray2>::value));

  // Test combinations of std::vector and C-style arrays.
  EXPECT_TRUE((IsFungible<IntVector, IntCArray1>::value));
  EXPECT_TRUE((IsFungible<IntVector, IntCArray2>::value));
  EXPECT_TRUE((IsFungible<FloatVector, FloatCArray1>::value));
  EXPECT_FALSE((IsFungible<IntVector, FloatCArray1>::value));
  EXPECT_FALSE((IsFungible<IntVector, FloatCArray2>::value));

  // Test combinations of std::array and C-style arrays.
  EXPECT_TRUE((IsFungible<IntArray1, IntCArray1>::value));
  EXPECT_TRUE((IsFungible<IntArray2, IntCArray2>::value));
  EXPECT_FALSE((IsFungible<IntArray1, IntCArray2>::value));
  EXPECT_TRUE((IsFungible<FloatArray1, FloatCArray1>::value));
  EXPECT_FALSE((IsFungible<IntArray1, FloatCArray1>::value));
  EXPECT_FALSE((IsFungible<IntArray2, FloatCArray2>::value));
}

TEST(FungibleTests, Map) {
  using IntArray = std::array<int, 1>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  EXPECT_TRUE(
      (IsFungible<std::map<int, IntArray>, std::map<int, IntArray>>::value));
  EXPECT_TRUE(
      (IsFungible<std::map<int, IntArray>, std::map<int, IntVector>>::value));
  EXPECT_FALSE(
      (IsFungible<std::map<int, IntArray>, std::map<int, FloatVector>>::value));
  EXPECT_FALSE(
      (IsFungible<std::map<int, IntArray>, std::map<float, IntArray>>::value));
  EXPECT_FALSE(
      (IsFungible<std::map<int, IntArray>, std::map<float, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::map<int, IntArray>,
                           std::map<float, FloatVector>>::value));
}

TEST(FungibleTests, UnorderedMap) {
  using IntArray = std::array<int, 1>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  EXPECT_TRUE((IsFungible<std::unordered_map<int, IntArray>,
                          std::unordered_map<int, IntArray>>::value));
  EXPECT_TRUE((IsFungible<std::unordered_map<int, IntArray>,
                          std::unordered_map<int, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::unordered_map<int, FloatVector>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::unordered_map<float, IntArray>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::unordered_map<float, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::unordered_map<float, FloatVector>>::value));
}

TEST(FungibleTests, MapUnorderedMap) {
  using IntArray = std::array<int, 1>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  EXPECT_TRUE((IsFungible<std::map<int, IntArray>,
                          std::unordered_map<int, IntArray>>::value));
  EXPECT_TRUE((IsFungible<std::map<int, IntArray>,
                          std::unordered_map<int, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::map<int, IntArray>,
                           std::unordered_map<int, FloatVector>>::value));
  EXPECT_FALSE((IsFungible<std::map<int, IntArray>,
                           std::unordered_map<float, IntArray>>::value));
  EXPECT_FALSE((IsFungible<std::map<int, IntArray>,
                           std::unordered_map<float, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::map<int, IntArray>,
                           std::unordered_map<float, FloatVector>>::value));

  EXPECT_TRUE((IsFungible<std::unordered_map<int, IntArray>,
                          std::map<int, IntArray>>::value));
  EXPECT_TRUE((IsFungible<std::unordered_map<int, IntArray>,
                          std::map<int, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::map<int, FloatVector>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::map<float, IntArray>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::map<float, IntVector>>::value));
  EXPECT_FALSE((IsFungible<std::unordered_map<int, IntArray>,
                           std::map<float, FloatVector>>::value));
}

TEST(FungibleTests, Optional) {
  // Test basic type combinations.
  EXPECT_TRUE((IsFungible<Optional<int>, Optional<int>>::value));
  EXPECT_TRUE((IsFungible<Optional<float>, Optional<float>>::value));
  EXPECT_FALSE((IsFungible<Optional<int>, Optional<float>>::value));
  EXPECT_FALSE((IsFungible<Optional<float>, Optional<int>>::value));

  using IntArray1 = std::array<int, 1>;
  using IntArray2 = std::array<int, 2>;
  using FloatArray1 = std::array<float, 1>;
  using FloatArray2 = std::array<float, 2>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  // Test combinations of more complex nested types.
  EXPECT_TRUE((IsFungible<Optional<IntVector>, Optional<IntVector>>::value));
  EXPECT_TRUE(
      (IsFungible<Optional<FloatVector>, Optional<FloatVector>>::value));
  EXPECT_TRUE((IsFungible<Optional<IntVector>, Optional<IntArray1>>::value));
  EXPECT_TRUE((IsFungible<Optional<IntVector>, Optional<IntArray2>>::value));
  EXPECT_FALSE((IsFungible<Optional<IntVector>, Optional<FloatArray1>>::value));
  EXPECT_FALSE((IsFungible<Optional<IntVector>, Optional<FloatArray2>>::value));
  EXPECT_TRUE(
      (IsFungible<Optional<FloatVector>, Optional<FloatArray1>>::value));
  EXPECT_TRUE(
      (IsFungible<Optional<FloatVector>, Optional<FloatArray2>>::value));
  EXPECT_FALSE((IsFungible<Optional<FloatVector>, Optional<IntArray1>>::value));
  EXPECT_FALSE((IsFungible<Optional<FloatVector>, Optional<IntArray2>>::value));
}

TEST(FungibleTests, Tuple) {
  using A = int;
  using B = float;

  EXPECT_TRUE((IsFungible<std::tuple<A>, std::tuple<A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A>, std::tuple<B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<B>, std::tuple<A>>::value));
  EXPECT_TRUE((IsFungible<std::tuple<B>, std::tuple<B>>::value));

  EXPECT_TRUE((IsFungible<std::tuple<A, A>, std::tuple<A, A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A>, std::tuple<B, A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A>, std::tuple<A, B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A>, std::tuple<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<B, A>, std::tuple<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, B>, std::tuple<B, B>>::value));
  EXPECT_TRUE((IsFungible<std::tuple<B, B>, std::tuple<B, B>>::value));

  EXPECT_FALSE((IsFungible<std::tuple<>, std::tuple<A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A>, std::tuple<A, A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A>, std::tuple<A, A, A>>::value));

  EXPECT_TRUE((IsFungible<std::tuple<>, std::tuple<>>::value));
}

TEST(FungibleTests, Pair) {
  using A = int;
  using B = float;

  // Test basic combinations.
  EXPECT_TRUE((IsFungible<std::pair<A, A>, std::pair<A, A>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, A>, std::pair<B, A>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, A>, std::pair<A, B>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, A>, std::pair<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::pair<B, A>, std::pair<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, B>, std::pair<B, B>>::value));
  EXPECT_TRUE((IsFungible<std::pair<B, B>, std::pair<B, B>>::value));
}

TEST(FungibleTests, PairTuple) {
  using A = int;
  using B = float;

  // Test basic combinations.
  EXPECT_TRUE((IsFungible<std::tuple<A, A>, std::pair<A, A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A>, std::pair<B, A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A>, std::pair<A, B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A>, std::pair<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<B, A>, std::pair<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, B>, std::pair<B, B>>::value));
  EXPECT_TRUE((IsFungible<std::tuple<B, B>, std::pair<B, B>>::value));

  EXPECT_TRUE((IsFungible<std::pair<A, A>, std::tuple<A, A>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, A>, std::tuple<B, A>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, A>, std::tuple<A, B>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, A>, std::tuple<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::pair<B, A>, std::tuple<B, B>>::value));
  EXPECT_FALSE((IsFungible<std::pair<A, B>, std::tuple<B, B>>::value));
  EXPECT_TRUE((IsFungible<std::pair<B, B>, std::tuple<B, B>>::value));

  using IntArray = std::array<int, 1>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  // Test combinations of fungible and non-fungible types.
  EXPECT_TRUE((IsFungible<std::tuple<IntArray, IntArray>,
                          std::pair<IntArray, IntVector>>::value));
  EXPECT_TRUE((IsFungible<std::tuple<IntArray, IntArray>,
                          std::pair<IntVector, IntArray>>::value));
  EXPECT_TRUE((IsFungible<std::tuple<IntArray, IntArray>,
                          std::pair<IntVector, IntVector>>::value));

  EXPECT_FALSE((IsFungible<std::tuple<IntArray, IntArray>,
                           std::pair<IntArray, FloatVector>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<IntArray, IntArray>,
                           std::pair<FloatVector, IntArray>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<IntArray, IntArray>,
                           std::pair<FloatVector, FloatVector>>::value));

  // Test mismatching element counts.
  EXPECT_FALSE((IsFungible<std::tuple<A>, std::pair<A, A>>::value));
  EXPECT_FALSE((IsFungible<std::tuple<A, A, A>, std::pair<A, A>>::value));
}

TEST(FungibleTests, Variant) {
  EXPECT_TRUE((IsFungible<Variant<int>, Variant<int>>::value));
  EXPECT_FALSE((IsFungible<Variant<float>, Variant<int>>::value));

  using IntArray = std::array<int, 1>;
  using FloatArray = std::array<float, 1>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  EXPECT_TRUE((IsFungible<Variant<IntArray, FloatArray>,
                          Variant<IntArray, FloatArray>>::value));
  EXPECT_FALSE((IsFungible<Variant<IntArray, FloatArray>,
                           Variant<FloatArray, IntArray>>::value));
  EXPECT_TRUE((IsFungible<Variant<IntArray, FloatArray>,
                          Variant<IntArray, FloatVector>>::value));
  EXPECT_FALSE((IsFungible<Variant<IntArray, FloatArray>,
                           Variant<FloatVector, IntArray>>::value));
  EXPECT_TRUE((IsFungible<Variant<IntArray, FloatArray>,
                          Variant<IntVector, FloatArray>>::value));
  EXPECT_FALSE((IsFungible<Variant<IntArray, FloatArray>,
                           Variant<FloatArray, IntVector>>::value));
  EXPECT_TRUE((IsFungible<Variant<IntArray, FloatArray>,
                          Variant<IntVector, FloatVector>>::value));
  EXPECT_FALSE((IsFungible<Variant<IntArray, FloatArray>,
                           Variant<FloatVector, IntVector>>::value));

  EXPECT_TRUE((IsFungible<Variant<>, Variant<>>::value));
  EXPECT_FALSE((IsFungible<Variant<>, Variant<int>>::value));
  EXPECT_FALSE((IsFungible<Variant<int>, Variant<int, int>>::value));
  EXPECT_FALSE((IsFungible<Variant<int, int>, Variant<int>>::value));
  EXPECT_FALSE((IsFungible<Variant<int>, Variant<>>::value));
}

TEST(FungibleTests, Signature) {
  using A = int;
  using B = float;

  EXPECT_TRUE((IsFungible<A(A, A), A(A, A)>::value));
  EXPECT_FALSE((IsFungible<A(A, A), B(A, A)>::value));
  EXPECT_FALSE((IsFungible<A(A, A), A(B, A)>::value));
  EXPECT_FALSE((IsFungible<A(A, A), A(A, B)>::value));

  using IntArray = std::array<int, 1>;
  using IntVector = std::vector<int>;
  using FloatVector = std::vector<float>;

  EXPECT_TRUE((IsFungible<void(IntArray), void(IntVector)>::value));
  EXPECT_FALSE((IsFungible<void(IntArray), void(FloatVector)>::value));
  EXPECT_TRUE((IsFungible<IntArray(void), IntVector(void)>::value));
  EXPECT_FALSE((IsFungible<IntArray(void), FloatVector(void)>::value));
  EXPECT_TRUE((IsFungible<IntArray(IntArray), IntVector(IntVector)>::value));
  EXPECT_FALSE((IsFungible<IntArray(IntArray), FloatVector(IntVector)>::value));
  EXPECT_FALSE((IsFungible<IntArray(IntArray), IntVector(FloatVector)>::value));
  EXPECT_FALSE(
      (IsFungible<IntArray(IntArray), FloatVector(FloatVector)>::value));
}

TEST(FungibleTests, LogicalBuffer) {
  using A = LogicalBuffer<int[10], std::size_t>;
  using B = std::vector<int>;
  using C = LogicalBuffer<float[10], std::size_t>;
  using D = std::vector<float>;
  using E = LogicalBuffer<std::array<int, 10>, std::size_t>;

  EXPECT_TRUE((IsFungible<A, A>::value));
  EXPECT_TRUE((IsFungible<A, B>::value));
  EXPECT_TRUE((IsFungible<B, A>::value));
  EXPECT_TRUE((IsFungible<B, B>::value));
  EXPECT_TRUE((IsFungible<A, E>::value));
  EXPECT_TRUE((IsFungible<E, A>::value));

  EXPECT_FALSE((IsFungible<A, C>::value));
  EXPECT_FALSE((IsFungible<C, A>::value));
  EXPECT_FALSE((IsFungible<A, D>::value));
  EXPECT_FALSE((IsFungible<D, A>::value));
}

namespace {

enum class ErrorA {
  None,
  Foo,
  Fuz,
};
template <typename T>
using ResultA = Result<ErrorA, T>;

enum class ErrorB {
  None,
  Bar,
  Baz,
};
template <typename T>
using ResultB = Result<ErrorB, T>;

}  // anonymous namespace

TEST(FungibleTests, Result) {
  // Result<EnumA, A> and Result<EnumA, B> are fungible if A and B are fungible.
  EXPECT_TRUE((IsFungible<ResultA<int>, ResultA<int>>::value));
  EXPECT_FALSE((IsFungible<ResultA<int>, ResultA<float>>::value));
  EXPECT_FALSE((IsFungible<ResultA<float>, ResultA<int>>::value));
  EXPECT_TRUE((IsFungible<ResultA<float>, ResultA<float>>::value));

  // Result<EnumB, A> and Result<EnumB, B> are fungible if A and B are fungible.
  EXPECT_TRUE((IsFungible<ResultB<int>, ResultB<int>>::value));
  EXPECT_FALSE((IsFungible<ResultB<int>, ResultB<float>>::value));
  EXPECT_FALSE((IsFungible<ResultB<float>, ResultB<int>>::value));
  EXPECT_TRUE((IsFungible<ResultB<float>, ResultB<float>>::value));

  // Result<EnumA, A> and Result<EnumB, B> are not fungible if EnumA and EnumB
  // are different, regardless of A and B being fungible.
  EXPECT_FALSE((IsFungible<ResultA<int>, ResultB<int>>::value));
  EXPECT_FALSE((IsFungible<ResultA<int>, ResultB<float>>::value));
  EXPECT_FALSE((IsFungible<ResultA<float>, ResultB<int>>::value));
  EXPECT_FALSE((IsFungible<ResultA<float>, ResultB<float>>::value));
}

namespace {

struct UserDefinedA {
  std::array<int, 10> a;
  std::array<float, 20> b;
  NOP_STRUCTURE(UserDefinedA, a, b);
};

struct UserDefinedB1 {
  std::array<int, 10> a;
  float b[20];
  NOP_STRUCTURE(UserDefinedB1, a, b);
};

struct UserDefinedB2 {
  int a[10];
  float b[20];
  NOP_STRUCTURE(UserDefinedB2, a, b);
};

struct UserDefinedC {
  int a[10];
  std::string b;
  NOP_STRUCTURE(UserDefinedC, a, b);
};

struct UserDefinedD {
  int a[10];
  NOP_STRUCTURE(UserDefinedD, a);
};

struct UserDefinedE {
  int a[10];
  float b[20];
  int c;
  NOP_STRUCTURE(UserDefinedE, a, b, c);
};

struct UserDefinedF1 {
  std::vector<int> a;
  NOP_STRUCTURE(UserDefinedF1, a);
};

struct UserDefinedF2 {
  int a[10];
  int b;
  NOP_STRUCTURE(UserDefinedF2, (a, b));
};

struct UserDefinedF3 {
  std::array<int, 10> a;
  int b;
  NOP_STRUCTURE(UserDefinedF3, (a, b));
};

}  // anonymous namespace

TEST(FungibleTests, UserDefined) {
  EXPECT_TRUE((IsFungible<UserDefinedA, UserDefinedA>::value));
  EXPECT_TRUE((IsFungible<UserDefinedA, UserDefinedB1>::value));
  EXPECT_TRUE((IsFungible<UserDefinedA, UserDefinedB2>::value));
  EXPECT_TRUE((IsFungible<UserDefinedB1, UserDefinedA>::value));
  EXPECT_TRUE((IsFungible<UserDefinedB2, UserDefinedA>::value));
  EXPECT_TRUE((IsFungible<UserDefinedB1, UserDefinedB1>::value));
  EXPECT_TRUE((IsFungible<UserDefinedB1, UserDefinedB2>::value));
  EXPECT_TRUE((IsFungible<UserDefinedB2, UserDefinedB1>::value));
  EXPECT_TRUE((IsFungible<UserDefinedB2, UserDefinedB2>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF1, UserDefinedF1>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF1, UserDefinedF2>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF1, UserDefinedF3>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF2, UserDefinedF1>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF2, UserDefinedF2>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF2, UserDefinedF3>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF3, UserDefinedF1>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF3, UserDefinedF2>::value));
  EXPECT_TRUE((IsFungible<UserDefinedF3, UserDefinedF3>::value));

  EXPECT_FALSE((IsFungible<UserDefinedA, UserDefinedC>::value));
  EXPECT_FALSE((IsFungible<UserDefinedA, UserDefinedD>::value));
  EXPECT_FALSE((IsFungible<UserDefinedA, UserDefinedE>::value));
  EXPECT_FALSE((IsFungible<UserDefinedC, UserDefinedA>::value));
  EXPECT_FALSE((IsFungible<UserDefinedD, UserDefinedA>::value));
  EXPECT_FALSE((IsFungible<UserDefinedE, UserDefinedA>::value));
}

TEST(FungibleTests, Entry) {
  using A0 = Entry<int, 0>;
  using A1 = Entry<int, 1>;
  using B0 = Entry<float, 0>;
  using B1 = Entry<float, 1>;
  using C0 = Entry<std::vector<int>, 0>;
  using C1 = Entry<std::vector<int>, 1>;
  using D0 = Entry<std::array<int, 2>, 0>;
  using D1 = Entry<std::array<int, 2>, 1>;

  EXPECT_TRUE((IsFungible<A0, A0>::value));
  EXPECT_FALSE((IsFungible<A0, A1>::value));
  EXPECT_FALSE((IsFungible<A0, B0>::value));
  EXPECT_FALSE((IsFungible<A0, B1>::value));
  EXPECT_FALSE((IsFungible<A0, C0>::value));
  EXPECT_FALSE((IsFungible<A0, C1>::value));
  EXPECT_FALSE((IsFungible<A0, D0>::value));
  EXPECT_FALSE((IsFungible<A0, D1>::value));

  EXPECT_FALSE((IsFungible<A1, A0>::value));
  EXPECT_TRUE((IsFungible<A1, A1>::value));
  EXPECT_FALSE((IsFungible<A1, B0>::value));
  EXPECT_FALSE((IsFungible<A1, B1>::value));
  EXPECT_FALSE((IsFungible<A1, C0>::value));
  EXPECT_FALSE((IsFungible<A1, C1>::value));
  EXPECT_FALSE((IsFungible<A1, D0>::value));
  EXPECT_FALSE((IsFungible<A1, D1>::value));

  EXPECT_FALSE((IsFungible<B0, A0>::value));
  EXPECT_FALSE((IsFungible<B0, A1>::value));
  EXPECT_TRUE((IsFungible<B0, B0>::value));
  EXPECT_FALSE((IsFungible<B0, B1>::value));
  EXPECT_FALSE((IsFungible<B0, C0>::value));
  EXPECT_FALSE((IsFungible<B0, C1>::value));
  EXPECT_FALSE((IsFungible<B0, D0>::value));
  EXPECT_FALSE((IsFungible<B0, D1>::value));

  EXPECT_FALSE((IsFungible<B1, A0>::value));
  EXPECT_FALSE((IsFungible<B1, A1>::value));
  EXPECT_FALSE((IsFungible<B1, B0>::value));
  EXPECT_TRUE((IsFungible<B1, B1>::value));
  EXPECT_FALSE((IsFungible<B1, C0>::value));
  EXPECT_FALSE((IsFungible<B1, C1>::value));
  EXPECT_FALSE((IsFungible<B1, D0>::value));
  EXPECT_FALSE((IsFungible<B1, D1>::value));

  EXPECT_FALSE((IsFungible<C0, A0>::value));
  EXPECT_FALSE((IsFungible<C0, A1>::value));
  EXPECT_FALSE((IsFungible<C0, B0>::value));
  EXPECT_FALSE((IsFungible<C0, B1>::value));
  EXPECT_TRUE((IsFungible<C0, C0>::value));
  EXPECT_FALSE((IsFungible<C0, C1>::value));
  EXPECT_TRUE((IsFungible<C0, D0>::value));
  EXPECT_FALSE((IsFungible<C0, D1>::value));

  EXPECT_FALSE((IsFungible<C1, A0>::value));
  EXPECT_FALSE((IsFungible<C1, A1>::value));
  EXPECT_FALSE((IsFungible<C1, B0>::value));
  EXPECT_FALSE((IsFungible<C1, B1>::value));
  EXPECT_FALSE((IsFungible<C1, C0>::value));
  EXPECT_TRUE((IsFungible<C1, C1>::value));
  EXPECT_FALSE((IsFungible<C1, D0>::value));
  EXPECT_TRUE((IsFungible<C1, D1>::value));

  EXPECT_FALSE((IsFungible<D0, A0>::value));
  EXPECT_FALSE((IsFungible<D0, A1>::value));
  EXPECT_FALSE((IsFungible<D0, B0>::value));
  EXPECT_FALSE((IsFungible<D0, B1>::value));
  EXPECT_TRUE((IsFungible<D0, C0>::value));
  EXPECT_FALSE((IsFungible<D0, C1>::value));
  EXPECT_TRUE((IsFungible<D0, D0>::value));
  EXPECT_FALSE((IsFungible<D0, D1>::value));

  EXPECT_FALSE((IsFungible<D1, A0>::value));
  EXPECT_FALSE((IsFungible<D1, A1>::value));
  EXPECT_FALSE((IsFungible<D1, B0>::value));
  EXPECT_FALSE((IsFungible<D1, B1>::value));
  EXPECT_FALSE((IsFungible<D1, C0>::value));
  EXPECT_TRUE((IsFungible<D1, C1>::value));
  EXPECT_FALSE((IsFungible<D1, D0>::value));
  EXPECT_TRUE((IsFungible<D1, D1>::value));
}

namespace {

struct TableA0 {
  Entry<int, 0> a;
  Entry<std::vector<int>, 1> b;
  NOP_TABLE_NS("TableA", TableA0, a, b);
};

struct TableA1 {
  Entry<int, 0> a;
  Entry<std::array<int, 10>, 1> b;
  NOP_TABLE_NS("TableA", TableA1, a, b);
};

struct TableA2 {
  Entry<int, 0> a;
  Entry<std::vector<int>, 1> b;
  NOP_TABLE_NS("DifferentLabel", TableA2, a, b);
};

struct TableA3 {
  Entry<int, 0> a;
  Entry<std::array<int, 10>, 1> b;
  Entry<float, 2> c;
  NOP_TABLE_NS("TableA", TableA3, a, b, c);
};

}  // anonymous namespace

TEST(FungibleTests, Table) {
  EXPECT_TRUE((IsFungible<TableA0, TableA0>::value));
  EXPECT_TRUE((IsFungible<TableA0, TableA1>::value));
  EXPECT_TRUE((IsFungible<TableA1, TableA0>::value));
  EXPECT_TRUE((IsFungible<TableA1, TableA1>::value));
  EXPECT_TRUE((IsFungible<TableA2, TableA2>::value));
  EXPECT_TRUE((IsFungible<TableA3, TableA3>::value));

  EXPECT_FALSE((IsFungible<TableA0, TableA2>::value));
  EXPECT_FALSE((IsFungible<TableA0, TableA3>::value));
  EXPECT_FALSE((IsFungible<TableA1, TableA2>::value));
  EXPECT_FALSE((IsFungible<TableA1, TableA3>::value));
  EXPECT_FALSE((IsFungible<TableA2, TableA0>::value));
  EXPECT_FALSE((IsFungible<TableA2, TableA1>::value));
  EXPECT_FALSE((IsFungible<TableA2, TableA3>::value));
  EXPECT_FALSE((IsFungible<TableA3, TableA0>::value));
  EXPECT_FALSE((IsFungible<TableA3, TableA1>::value));
  EXPECT_FALSE((IsFungible<TableA3, TableA2>::value));
}

namespace {

template <typename T>
struct ValueWrapper {
  T value;
  NOP_VALUE(ValueWrapper, value);
};

template <typename T, std::size_t Length>
struct ArrayWrapper {
  std::array<T, Length> data;
  std::size_t size;
  NOP_VALUE(ArrayWrapper, (data, size));
};

}  // anonymous namespace

TEST(FungibleTests, Value) {
  using IntWrapper = ValueWrapper<int>;
  using FloatWrapper = ValueWrapper<float>;
  using StringWrapper = ValueWrapper<std::string>;

  using IntArray1 = ArrayWrapper<int, 1>;
  using IntArray2 = ArrayWrapper<int, 2>;
  using FloatArray1 = ArrayWrapper<float, 1>;
  using FloatArray2 = ArrayWrapper<float, 2>;
  using StringArray1 = ArrayWrapper<std::string, 1>;
  using StringArray2 = ArrayWrapper<std::string, 2>;

  EXPECT_TRUE((IsFungible<IntWrapper, IntWrapper>::value));
  EXPECT_TRUE((IsFungible<FloatWrapper, FloatWrapper>::value));
  EXPECT_TRUE((IsFungible<StringWrapper, StringWrapper>::value));

  EXPECT_TRUE((IsFungible<IntArray1, IntArray1>::value));
  EXPECT_TRUE((IsFungible<IntArray2, IntArray2>::value));
  EXPECT_TRUE((IsFungible<FloatArray1, FloatArray1>::value));
  EXPECT_TRUE((IsFungible<FloatArray2, FloatArray2>::value));
  EXPECT_TRUE((IsFungible<StringArray1, StringArray1>::value));
  EXPECT_TRUE((IsFungible<StringArray2, StringArray2>::value));

  EXPECT_TRUE((IsFungible<IntWrapper, int>::value));
  EXPECT_TRUE((IsFungible<int, IntWrapper>::value));
  EXPECT_TRUE((IsFungible<FloatWrapper, float>::value));
  EXPECT_TRUE((IsFungible<float, FloatWrapper>::value));
  EXPECT_TRUE((IsFungible<StringWrapper, std::string>::value));
  EXPECT_TRUE((IsFungible<std::string, StringWrapper>::value));

  EXPECT_TRUE((IsFungible<IntArray1, std::vector<int>>::value));
  EXPECT_TRUE((IsFungible<std::vector<int>, IntArray1>::value));
  EXPECT_TRUE((IsFungible<IntArray2, std::vector<int>>::value));
  EXPECT_TRUE((IsFungible<std::vector<int>, IntArray2>::value));
  EXPECT_TRUE((IsFungible<FloatArray1, std::vector<float>>::value));
  EXPECT_TRUE((IsFungible<std::vector<float>, FloatArray1>::value));
  EXPECT_TRUE((IsFungible<FloatArray2, std::vector<float>>::value));
  EXPECT_TRUE((IsFungible<std::vector<float>, FloatArray2>::value));
  EXPECT_TRUE((IsFungible<StringArray1, std::vector<std::string>>::value));
  EXPECT_TRUE((IsFungible<std::vector<std::string>, StringArray1>::value));
  EXPECT_TRUE((IsFungible<StringArray2, std::vector<std::string>>::value));
  EXPECT_TRUE((IsFungible<std::vector<std::string>, StringArray2>::value));

  EXPECT_FALSE((IsFungible<IntWrapper, FloatWrapper>::value));
  EXPECT_FALSE((IsFungible<IntWrapper, StringWrapper>::value));
  EXPECT_FALSE((IsFungible<FloatWrapper, IntWrapper>::value));
  EXPECT_FALSE((IsFungible<FloatWrapper, StringWrapper>::value));
  EXPECT_FALSE((IsFungible<StringWrapper, IntWrapper>::value));
  EXPECT_FALSE((IsFungible<StringWrapper, FloatWrapper>::value));

  EXPECT_FALSE((IsFungible<IntWrapper, float>::value));
  EXPECT_FALSE((IsFungible<IntWrapper, std::string>::value));
  EXPECT_FALSE((IsFungible<float, IntWrapper>::value));
  EXPECT_FALSE((IsFungible<std::string, IntWrapper>::value));
  EXPECT_FALSE((IsFungible<FloatWrapper, int>::value));
  EXPECT_FALSE((IsFungible<FloatWrapper, std::string>::value));
  EXPECT_FALSE((IsFungible<int, FloatWrapper>::value));
  EXPECT_FALSE((IsFungible<std::string, FloatWrapper>::value));
  EXPECT_FALSE((IsFungible<StringWrapper, int>::value));
  EXPECT_FALSE((IsFungible<StringWrapper, float>::value));
  EXPECT_FALSE((IsFungible<int, StringWrapper>::value));
  EXPECT_FALSE((IsFungible<float, StringWrapper>::value));

  EXPECT_FALSE((IsFungible<IntArray1, std::vector<float>>::value));
  EXPECT_FALSE((IsFungible<IntArray1, std::vector<std::string>>::value));
  EXPECT_FALSE((IsFungible<std::vector<float>, IntArray1>::value));
  EXPECT_FALSE((IsFungible<std::vector<std::string>, IntArray1>::value));
  EXPECT_FALSE((IsFungible<IntArray2, std::vector<float>>::value));
  EXPECT_FALSE((IsFungible<IntArray2, std::vector<std::string>>::value));
  EXPECT_FALSE((IsFungible<std::vector<float>, IntArray2>::value));
  EXPECT_FALSE((IsFungible<std::vector<std::string>, IntArray2>::value));
  EXPECT_FALSE((IsFungible<FloatArray1, std::vector<int>>::value));
  EXPECT_FALSE((IsFungible<FloatArray1, std::vector<std::string>>::value));
  EXPECT_FALSE((IsFungible<std::vector<int>, FloatArray1>::value));
  EXPECT_FALSE((IsFungible<std::vector<std::string>, FloatArray1>::value));
  EXPECT_FALSE((IsFungible<FloatArray2, std::vector<int>>::value));
  EXPECT_FALSE((IsFungible<FloatArray2, std::vector<std::string>>::value));
  EXPECT_FALSE((IsFungible<std::vector<int>, FloatArray2>::value));
  EXPECT_FALSE((IsFungible<std::vector<std::string>, FloatArray2>::value));
  EXPECT_FALSE((IsFungible<StringArray1, std::vector<int>>::value));
  EXPECT_FALSE((IsFungible<StringArray1, std::vector<float>>::value));
  EXPECT_FALSE((IsFungible<std::vector<int>, StringArray1>::value));
  EXPECT_FALSE((IsFungible<std::vector<float>, StringArray1>::value));
  EXPECT_FALSE((IsFungible<StringArray2, std::vector<int>>::value));
  EXPECT_FALSE((IsFungible<StringArray2, std::vector<float>>::value));
  EXPECT_FALSE((IsFungible<std::vector<int>, StringArray2>::value));
  EXPECT_FALSE((IsFungible<std::vector<float>, StringArray2>::value));
}
