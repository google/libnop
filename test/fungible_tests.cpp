#include <gtest/gtest.h>

#include <array>

#include <nop/traits/is_fungible.h>
#include <nop/types/optional.h>
#include <nop/types/result.h>
#include <nop/types/variant.h>

using nop::IsFungible;
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
