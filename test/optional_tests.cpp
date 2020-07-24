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

#include <algorithm>
#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include <nop/types/optional.h>

using nop::InPlace;
using nop::Optional;

namespace {

template <typename T>
class TestType {
 public:
  TestType(const T& value) : value_(value) {}
  TestType(T&& value) : value_(std::move(value)) {}
  TestType(const TestType&) = default;
  TestType(TestType&&) = default;

  TestType& operator=(const TestType&) = default;
  TestType& operator=(TestType&&) = default;

  const T& get() const { return value_; }
  T&& take() { return std::move(value_); }

 private:
  T value_;
};

template <typename T>
class InstrumentType {
 public:
  InstrumentType(const T& value) : value_(value) { constructor_count_++; }
  InstrumentType(T&& value) : value_(std::move(value)) { constructor_count_++; }
  InstrumentType(const InstrumentType& other) : value_(other.value_) {
    constructor_count_++;
  }
  InstrumentType(InstrumentType&& other) : value_(std::move(other.value_)) {
    constructor_count_++;
  }
  InstrumentType(const TestType<T>& other) : value_(other.get()) {
    constructor_count_++;
  }
  InstrumentType(TestType<T>&& other) : value_(other.take()) {
    constructor_count_++;
  }
  ~InstrumentType() { destructor_count_++; }

  InstrumentType& operator=(const InstrumentType& other) {
    copy_assignment_count_++;
    value_ = other.value_;
    return *this;
  }
  InstrumentType& operator=(InstrumentType&& other) {
    move_assignment_count_++;
    value_ = std::move(other.value_);
    return *this;
  }

  InstrumentType& operator=(const TestType<T>& other) {
    copy_assignment_count_++;
    value_ = other.get();
    return *this;
  }
  InstrumentType& operator=(TestType<T>&& other) {
    move_assignment_count_++;
    value_ = other.take();
    return *this;
  }

  static std::size_t constructor_count() { return constructor_count_; }
  static std::size_t destructor_count() { return destructor_count_; }
  static std::size_t move_assignment_count() { return move_assignment_count_; }
  static std::size_t copy_assignment_count() { return copy_assignment_count_; }

  const T& get() const { return value_; }
  T&& take() { return std::move(value_); }

  static void clear() {
    constructor_count_ = 0;
    destructor_count_ = 0;
    move_assignment_count_ = 0;
    copy_assignment_count_ = 0;
  }

 private:
  T value_;

  static std::size_t constructor_count_;
  static std::size_t destructor_count_;
  static std::size_t move_assignment_count_;
  static std::size_t copy_assignment_count_;
};

template <typename T>
std::size_t InstrumentType<T>::constructor_count_ = 0;
template <typename T>
std::size_t InstrumentType<T>::destructor_count_ = 0;
template <typename T>
std::size_t InstrumentType<T>::move_assignment_count_ = 0;
template <typename T>
std::size_t InstrumentType<T>::copy_assignment_count_ = 0;

// Type to test relational operator enables on subclasses of Optional.
template <typename T>
struct OptionalSubclass : Optional<T> {
  using Optional<T>::Optional;

  constexpr bool has_value() const { return !this->empty(); }
};

}  // anonymous namespace

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

  {
    using Pair = std::pair<int, int>;
    const Pair pair{10, 20};
    Optional<Pair> value{pair};
    ASSERT_FALSE(value.empty());

    const Pair expected{10, 20};
    EXPECT_EQ(expected, value.get());
  }

  {
    using Pair = std::pair<int, int>;
    const Optional<Pair> pair{InPlace{}, 10, 20};
    Optional<Pair> value{pair};
    ASSERT_FALSE(value.empty());

    const Pair expected{10, 20};
    EXPECT_EQ(expected, value.get());
  }

  {
    using Pair = std::pair<int, int>;
    Optional<Pair> pair{InPlace{}, 10, 20};
    Optional<Pair> value{std::move(pair)};
    ASSERT_FALSE(value.empty());

    const Pair expected{10, 20};
    EXPECT_EQ(expected, value.get());
    EXPECT_FALSE(pair.empty());
  }

  {
    using Pair = std::pair<std::string, std::string>;
    Optional<Pair> value{InPlace{}, "foo", "bar"};
    ASSERT_FALSE(value.empty());

    const Pair expected{"foo", "bar"};
    EXPECT_EQ(expected, value.get());
  }

  {
    Optional<std::vector<std::string>> value{
        InPlace{}, std::initializer_list<std::string>{"foo", "bar"}};
    ASSERT_FALSE(value.empty());

    const std::vector<std::string> expected{"foo", "bar"};
    EXPECT_EQ(expected, value.get());
  }

  {
    Optional<std::vector<std::string>> value{
        std::initializer_list<std::string>{"foo", "bar"}};
    ASSERT_FALSE(value.empty());

    const std::vector<std::string> expected{"foo", "bar"};
    EXPECT_EQ(expected, value.get());
  }

  {
    Optional<std::string> value{"foo"};
    ASSERT_FALSE(value.empty());
    ASSERT_FALSE(value.get().empty());

    const std::string s = value.take();
    ASSERT_FALSE(value.empty());
    EXPECT_TRUE(value.get().empty());
    EXPECT_EQ("foo", s);
  }
}

TEST(Optional, Assignment) {
  {
    Optional<int> v;
    EXPECT_TRUE(v.empty());
    v = 10;
    ASSERT_FALSE(v.empty());
    EXPECT_EQ(10, v.get());
  }

  {
    Optional<std::string> v;
    EXPECT_TRUE(v.empty());
    v = std::string("test");
    ASSERT_FALSE(v.empty());
    EXPECT_EQ("test", v.get());
  }

  {
    Optional<std::string> v;
    EXPECT_TRUE(v.empty());
    v = "test";
    ASSERT_FALSE(v.empty());
    EXPECT_EQ("test", v.get());
  }

  {
    std::string s{"test"};
    EXPECT_FALSE(s.empty());

    Optional<std::string> v;
    EXPECT_TRUE(v.empty());

    v = s;
    EXPECT_FALSE(s.empty());
    ASSERT_FALSE(v.empty());
    EXPECT_EQ("test", s);
    EXPECT_EQ("test", v.get());
  }

  {
    std::string s{"test"};
    EXPECT_FALSE(s.empty());

    Optional<std::string> v;
    EXPECT_TRUE(v.empty());

    v = std::move(s);
    EXPECT_TRUE(s.empty());
    ASSERT_FALSE(v.empty());
    EXPECT_EQ("", s);
    EXPECT_EQ("test", v.get());
  }

  {
    Optional<const char*> v1;
    Optional<std::string> v2;

    v1 = "test";
    ASSERT_FALSE(v1.empty());
    v2 = v1;
    ASSERT_FALSE(v1.empty());
    ASSERT_FALSE(v2.empty());
    EXPECT_EQ("test", v1.get());
    EXPECT_EQ("test", v2.get());
  }

  {
    Optional<int> v1{1};
    Optional<int> v2;
    ASSERT_FALSE(v1.empty());
    ASSERT_TRUE(v2.empty());

    v2 = v1;
    ASSERT_FALSE(v1.empty());
    ASSERT_FALSE(v2.empty());
    EXPECT_EQ(1, v1.get());
    EXPECT_EQ(1, v2.get());
  }

  {
    Optional<int> v1{1};
    Optional<int> v2;
    ASSERT_FALSE(v1.empty());
    ASSERT_TRUE(v2.empty());

    v1 = v2;
    ASSERT_TRUE(v1.empty());
    ASSERT_TRUE(v2.empty());
  }

  {
    Optional<int> v1{1};
    Optional<int> v2;
    ASSERT_FALSE(v1.empty());
    ASSERT_TRUE(v2.empty());

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_FALSE(v2.empty());
    EXPECT_EQ(1, v2.get());
  }

  {
    Optional<std::string> v1{"test"};
    Optional<std::string> v2;
    ASSERT_FALSE(v1.empty());
    ASSERT_TRUE(v2.empty());

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_FALSE(v2.empty());
    EXPECT_EQ("test", v2.get());
  }

  {
    Optional<std::string> v1{"test"};
    Optional<std::tuple<std::string>> v2;
    ASSERT_FALSE(v1.empty());
    ASSERT_TRUE(v2.empty());

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_FALSE(v2.empty());
    EXPECT_EQ(std::tuple<std::string>{"test"}, v2.get());
  }

  {
    Optional<std::string> v1;
    Optional<std::tuple<std::string>> v2{"test"};
    ASSERT_TRUE(v1.empty());
    ASSERT_FALSE(v2.empty());

    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_TRUE(v2.empty());
  }

  {
    Optional<std::string> v1{"test"};
    Optional<std::string> v2{"fizz"};
    ASSERT_FALSE(v1.empty());
    ASSERT_FALSE(v2.empty());

    // Should this leave v1 empty or the internal string empty?
    v2 = std::move(v1);
    ASSERT_TRUE(v1.empty());
    ASSERT_FALSE(v2.empty());
    EXPECT_EQ("test", v2.get());
  }

  {
    Optional<const char*> v1;
    Optional<std::string> v2{"foo"};
    ASSERT_TRUE(v1.empty());
    ASSERT_FALSE(v2.empty());

    v2 = v1;
    EXPECT_TRUE(v1.empty());
    EXPECT_TRUE(v2.empty());
  }
}

TEST(Optional, CopyMoveConstructAssign) {
  {
    InstrumentType<int>::clear();

    ASSERT_EQ(0u, InstrumentType<int>::constructor_count());
    ASSERT_EQ(0u, InstrumentType<int>::destructor_count());
    ASSERT_EQ(0u, InstrumentType<int>::move_assignment_count());
    ASSERT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Default construct to empty, no InstrumentType activity.
    Optional<InstrumentType<int>> v;
    EXPECT_EQ(0u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Optional<InstrumentType<int>> v;
    v = InstrumentType<int>(10);
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Copy construct.
    Optional<InstrumentType<int>> v1{10};
    Optional<InstrumentType<int>> v2{v1};
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Copy construct.
    const InstrumentType<int> i{10};
    Optional<InstrumentType<int>> v{i};
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Optional<InstrumentType<int>> v{InstrumentType<int>(10)};
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Optional<InstrumentType<int>> v{InstrumentType<int>(25)};

    // Assign from temporary, temporary ctor/dtor.
    v = InstrumentType<int>(35);
    EXPECT_EQ(3u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(2u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Optional<InstrumentType<int>> v{InstrumentType<int>(25)};

    // dtor.
    v.clear();
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(2u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Optional<InstrumentType<int>> v{InstrumentType<int>(25)};

    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }
  EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
  EXPECT_EQ(2u, InstrumentType<int>::destructor_count());
  EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
  EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Optional<InstrumentType<int>> v{TestType<int>(10)};
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Optional<InstrumentType<int>> v{TestType<int>(10)};
    // Assign from other temporary.
    v = TestType<int>(11);
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Optional<InstrumentType<int>> v{TestType<int>(10)};
    // Assign from empty Variant.
    v = Optional<InstrumentType<int>>();
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    TestType<int> other(10);
    // Construct from other.
    Optional<InstrumentType<int>> v{other};

    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Optional<InstrumentType<int>> v{TestType<int>(0)};
    TestType<int> other(10);
    // Assign from other.
    v = other;
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(1u, InstrumentType<int>::copy_assignment_count());
  }
}

TEST(Optional, MoveConstructor) {
  {
    std::unique_ptr<int> pointer = std::make_unique<int>(10);
    Optional<std::unique_ptr<int>> v(std::move(pointer));
    ASSERT_FALSE(v.empty());
    EXPECT_TRUE(v.get() != nullptr);
    EXPECT_TRUE(pointer == nullptr);
  }

  {
    Optional<std::unique_ptr<int>> a(std::make_unique<int>(10));
    Optional<std::unique_ptr<int>> b(std::move(a));

    ASSERT_FALSE(a.empty());
    ASSERT_FALSE(b.empty());
    EXPECT_TRUE(a.get() == nullptr);
    EXPECT_TRUE(b.get() != nullptr);
  }
}

TEST(Optional, RelationalOperators) {
  {
    const Optional<int> a{};
    const Optional<int> b{};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= b);
  }

  {
    const Optional<int> a{10};
    const Optional<int> b{10};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= b);
  }

  {
    const Optional<int> a{10};
    const Optional<int> b{};
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_TRUE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_FALSE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_TRUE(b < a);
    EXPECT_FALSE(b > a);
    EXPECT_FALSE(b >= a);
    EXPECT_TRUE(b <= a);
  }

  {
    const Optional<int> a{10};
    const Optional<int> b{20};

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);
  }

  {
    const Optional<int> a{10};
    const int b{10};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_TRUE(b == a);
    EXPECT_FALSE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b <= a);
  }

  {
    const Optional<int> a{10};
    const int b{20};

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);
  }

  {
    const Optional<int> a{};
    const int b{10};

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);
  }

  {
    const Optional<std::string> a{};
    const Optional<std::string> b{};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= b);
  }

  {
    const Optional<std::string> a{"bar"};
    const Optional<std::string> b{"bar"};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= b);
  }

  {
    const Optional<std::string> a{"bar"};
    const Optional<std::string> b{"foo"};

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);
  }

  {
    const Optional<std::string> a{"bar"};
    const std::string b{"bar"};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_TRUE(b == a);
    EXPECT_FALSE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b <= a);
  }

  {
    const Optional<std::string> a{"bar"};
    const std::string b{"foo"};

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);
  }

  {
    const Optional<std::string> a{};
    const std::string b{"bar"};

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a >= b);
    EXPECT_TRUE(a <= b);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(b <= a);
  }

  {
    OptionalSubclass<std::string> a{};
    OptionalSubclass<std::string> b{};

    EXPECT_TRUE(a == b);
  }
}

namespace {

struct TriviallyDestructible {
  int a;
};
static_assert(std::is_trivially_destructible<TriviallyDestructible>::value, "");

struct NonTriviallyDestructible {
  int a;
  ~NonTriviallyDestructible() { a = 0; }
};
static_assert(!std::is_trivially_destructible<NonTriviallyDestructible>::value,
              "");

}  // anonymous namespace

TEST(Optional, Trivial) {
  EXPECT_TRUE((std::is_trivially_destructible<Optional<int>>::value));
  EXPECT_TRUE((std::is_trivially_destructible<Optional<float>>::value));
  EXPECT_TRUE((std::is_trivially_destructible<Optional<float>>::value));
  EXPECT_TRUE(
      (std::is_trivially_destructible<Optional<TriviallyDestructible>>::value));

  EXPECT_FALSE((std::is_trivially_destructible<Optional<std::string>>::value));
  EXPECT_FALSE((std::is_trivially_destructible<
                Optional<NonTriviallyDestructible>>::value));
}

TEST(Optional, Constexpr) {
  constexpr Optional<int> constexpr_optional{10};
  constexpr bool value = constexpr_optional.get() == 10;
  static_assert(value, "");

  EXPECT_TRUE(value);
}
