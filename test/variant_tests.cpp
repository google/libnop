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

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>
#include <nop/types/variant.h>

using nop::detail::HasType;
using nop::detail::Set;
using nop::EmptyVariant;
using nop::IfAnyOf;
using nop::Variant;

namespace {

struct BaseType {
  BaseType(int value) : value{value} {}
  BaseType(const BaseType& other) : value{other.value} {}
  int value;
};

struct DerivedType : BaseType {
  DerivedType(int value) : BaseType{value} {};
  DerivedType(const BaseType& other) : BaseType{other} {}
};

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

}  // anonymous namespace

TEST(Variant, Assignment) {
  // Assert basic type properties.
  {
    Variant<int, bool, float> v;
    ASSERT_EQ(-1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_FALSE(v.is<float>());
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
  }

  {
    Variant<int, bool, float> v;
    v = 10;
    ASSERT_EQ(0, v.index());
    ASSERT_TRUE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_FALSE(v.is<float>());
    EXPECT_EQ(10, std::get<int>(v));
    EXPECT_NE(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_NE(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
  }

  {
    Variant<int, bool, float> v;
    v = false;
    ASSERT_EQ(1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_TRUE(v.is<bool>());
    ASSERT_FALSE(v.is<float>());
    EXPECT_EQ(false, std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_NE(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_NE(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
  }

  {
    Variant<int, bool, float> v;
    v = 1.0f;
    ASSERT_EQ(2, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_TRUE(v.is<float>());
    EXPECT_FLOAT_EQ(1.0f, std::get<float>(v));
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_NE(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_NE(nullptr, &std::get<float>(v));
  }

  {
    const Variant<int, bool, float> v;
    ASSERT_EQ(-1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_FALSE(v.is<float>());
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
  }

  {
    const Variant<int, bool, float> v{10};
    ASSERT_EQ(0, v.index());
    ASSERT_TRUE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_FALSE(v.is<float>());
    EXPECT_EQ(10, std::get<int>(v));
    EXPECT_NE(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_NE(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
  }

  {
    const Variant<int, bool, float> v{false};
    ASSERT_EQ(1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_TRUE(v.is<bool>());
    ASSERT_FALSE(v.is<float>());
    EXPECT_EQ(false, std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_NE(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_NE(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
  }

  {
    const Variant<int, bool, float> v{1.0f};
    ASSERT_EQ(2, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_TRUE(v.is<float>());
    EXPECT_FLOAT_EQ(1.0f, std::get<float>(v));
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_NE(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_NE(nullptr, &std::get<float>(v));
  }

  {
    Variant<int, bool, float> v;
    // ERROR: More than one type is implicitly convertible from double.
    // v = 1.0;
    v = static_cast<float>(1.0);
  }

  {
    Variant<int, bool, float> v;

    double x = 1.1;
    v = static_cast<float>(x);
    ASSERT_EQ(2, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_FALSE(v.is<bool>());
    ASSERT_TRUE(v.is<float>());
    EXPECT_FLOAT_EQ(1.1, std::get<float>(v));
  }

  {
    Variant<int, std::string> v;
    ASSERT_EQ(-1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_FALSE(v.is<std::string>());
  }

  {
    Variant<int, std::string> v;
    v = 20;
    ASSERT_EQ(0, v.index());
    ASSERT_TRUE(v.is<int>());
    ASSERT_FALSE(v.is<std::string>());
    EXPECT_EQ(20, std::get<int>(v));
  }

  {
    Variant<int, std::string> v;
    v = std::string("test");
    ASSERT_EQ(1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }

  {
    Variant<int, std::string> v;
    v = "test";
    ASSERT_EQ(1, v.index());
    ASSERT_FALSE(v.is<int>());
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }

  {
    Variant<const char*> v1;
    Variant<std::string> v2;

    v1 = "test";
    ASSERT_TRUE(v1.is<const char*>());
    v2 = v1;
    ASSERT_TRUE(v2.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v2));
  }

  {
    Variant<int> a(1);
    Variant<int> b;
    ASSERT_TRUE(!a.empty());
    ASSERT_TRUE(b.empty());

    a = b;
    ASSERT_TRUE(a.empty());
    ASSERT_TRUE(b.empty());
  }

  {
    Variant<int*, char*> v;

    // ERROR: More than one type is implicitly convertible from nullptr.
    // v = nullptr;

    v = static_cast<int*>(nullptr);
    EXPECT_TRUE(v.is<int*>());

    v = static_cast<char*>(nullptr);
    EXPECT_TRUE(v.is<char*>());
  }

  {
    Variant<int*, char*> v;
    int a = 10;
    char b = 20;

    v = &b;
    ASSERT_TRUE(v.is<char*>());
    EXPECT_EQ(&b, std::get<char*>(v));
    EXPECT_EQ(b, *std::get<char*>(v));

    v = &a;
    ASSERT_TRUE(v.is<int*>());
    EXPECT_EQ(&a, std::get<int*>(v));
    EXPECT_EQ(a, *std::get<int*>(v));
  }

  {
    using IntRef = std::reference_wrapper<int>;
    Variant<IntRef> v;
    int a = 10;

    v = a;
    ASSERT_TRUE(v.is<IntRef>());
    EXPECT_EQ(a, std::get<IntRef>(v));

    a = 20;
    EXPECT_EQ(a, std::get<IntRef>(v));
  }

  {
    Variant<std::string, int> v{"foo"};
    v = "test";

    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }

  {
    Variant<std::string, int> v{10};
    v = "test";

    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }
}

TEST(Variant, MoveAssignment) {
  {
    Variant<std::string> v;
    std::string s = "test";
    v = std::move(s);

    EXPECT_TRUE(s.empty());
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }

  {
    Variant<std::string, int> v{"foo"};
    std::string s = "test";
    v = std::move(s);

    EXPECT_TRUE(s.empty());
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }

  {
    Variant<std::string> v("test");
    std::string s = "fizz";
    s = std::move(std::get<std::string>(v));

    ASSERT_TRUE(v.is<std::string>());
    EXPECT_TRUE(std::get<std::string>(v).empty());
    EXPECT_EQ("test", s);
  }

  {
    Variant<std::string> a("test");
    Variant<std::string> b;

    b = std::move(a);
    ASSERT_TRUE(a.is<std::string>());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_TRUE(std::get<std::string>(a).empty());
    EXPECT_EQ("test", std::get<std::string>(b));
  }

  {
    Variant<std::string> a("test");
    Variant<std::string> b("fizz");

    b = std::move(a);
    ASSERT_TRUE(a.is<std::string>());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_TRUE(std::get<std::string>(a).empty());
    EXPECT_EQ("test", std::get<std::string>(b));
  }

  {
    Variant<const char*> a("test");
    Variant<std::string> b("fizz");

    b = std::move(a);
    ASSERT_TRUE(a.is<const char*>());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(b));
  }

  {
    Variant<int, std::string> a("test");
    Variant<int, std::string> b(10);

    b = std::move(a);
    ASSERT_TRUE(a.is<std::string>());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_TRUE(std::get<std::string>(a).empty());
    EXPECT_EQ("test", std::get<std::string>(b));
  }

  {
    Variant<int, std::string> a(10);
    Variant<int, std::string> b("test");

    b = std::move(a);
    ASSERT_TRUE(a.is<int>());
    ASSERT_TRUE(b.is<int>());
    EXPECT_EQ(10, std::get<int>(a));
    EXPECT_EQ(10, std::get<int>(b));
  }
}

TEST(Variant, Constructor) {
  {
    Variant<int, bool, float> v(true);
    EXPECT_TRUE(v.is<bool>());
  }

  {
    Variant<int, bool, float> v(10);
    EXPECT_TRUE(v.is<int>());
  }

  {
    Variant<int, bool, float> v(10.1f);
    EXPECT_TRUE(v.is<float>());
  }

  {
    Variant<float, std::string> v(10.);
    EXPECT_TRUE(v.is<float>());
  }

  {
    TestType<int> i(1);
    Variant<int, bool, float> v(i.take());
    ASSERT_TRUE(v.is<int>());
    EXPECT_EQ(1, std::get<int>(v));
  }

  {
    TestType<int> i(1);
    Variant<int, bool, float> v(i.get());
    ASSERT_TRUE(v.is<int>());
    EXPECT_EQ(1, std::get<int>(v));
  }

  {
    TestType<bool> b(true);
    Variant<int, bool, float> v(b.take());
    ASSERT_TRUE(v.is<bool>());
    EXPECT_EQ(true, std::get<bool>(v));
  }

  {
    TestType<bool> b(true);
    Variant<int, bool, float> v(b.get());
    ASSERT_TRUE(v.is<bool>());
    EXPECT_EQ(true, std::get<bool>(v));
  }

  {
    Variant<const char*> c("test");
    Variant<std::string> s(c);
    ASSERT_TRUE(s.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(s));
  }

  {
    const Variant<std::string> c("test");
    Variant<std::string> s(c);
    ASSERT_TRUE(s.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(s));
  }

  {
    Variant<int, bool, float> a(true);
    EXPECT_TRUE(a.is<bool>());
    Variant<int, bool, float> b(a);
    EXPECT_TRUE(b.is<bool>());
  }

  {
    using ArrayType = std::array<float, 3>;
    Variant<int, bool, ArrayType> a{ArrayType{{0.0f, 1.0f, 2.0f}}};
    EXPECT_TRUE(a.is<ArrayType>());

    a = EmptyVariant{};
    EXPECT_TRUE(a.empty());

    ArrayType array{{3.0f, 2.0f, 1.0f}};
    a = array;
    EXPECT_TRUE(a.is<ArrayType>());
  }

  {
    using IntRef = std::reference_wrapper<int>;
    int a = 10;
    Variant<IntRef> v(a);
    TestType<IntRef> t(a);

    ASSERT_TRUE(v.is<IntRef>());
    EXPECT_EQ(a, std::get<IntRef>(v));
    EXPECT_EQ(a, t.get());

    a = 20;
    EXPECT_EQ(a, std::get<IntRef>(v));
    EXPECT_EQ(a, t.get());
  }
}

// Verify correct ctor/dtor and assignment behavior used an instrumented type.
TEST(Variant, CopyMoveConstructAssign) {
  {
    InstrumentType<int>::clear();

    // Default construct to empty, no InstrumentType activity.
    Variant<int, InstrumentType<int>> v;
    ASSERT_EQ(0u, InstrumentType<int>::constructor_count());
    ASSERT_EQ(0u, InstrumentType<int>::destructor_count());
    ASSERT_EQ(0u, InstrumentType<int>::move_assignment_count());
    ASSERT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from int type, no InstrumentType activity.
    Variant<int, InstrumentType<int>> v;
    v = 10;
    EXPECT_EQ(0u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from int type, no InstrumentType activity.
    Variant<int, InstrumentType<int>> v(10);
    EXPECT_EQ(0u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Variant<int, InstrumentType<int>> v;
    v = InstrumentType<int>(25);
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Variant<int, InstrumentType<int>> v(InstrumentType<int>(25));
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Variant<int, InstrumentType<int>> v(InstrumentType<int>(25));

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
    Variant<int, InstrumentType<int>> v(InstrumentType<int>(25));

    // dtor.
    v = 10;
    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(2u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Variant<int, InstrumentType<int>> v(InstrumentType<int>(25));

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

    // Construct from empty.
    Variant<int, InstrumentType<int>> v(EmptyVariant{});

    EXPECT_EQ(0u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }
  EXPECT_EQ(0u, InstrumentType<int>::constructor_count());
  EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
  EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
  EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Variant<int, InstrumentType<int>> v(TestType<int>(10));
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Variant<int, InstrumentType<int>> v(TestType<int>(10));
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
    Variant<int, InstrumentType<int>> v(TestType<int>(10));
    // Assign from empty Variant.
    v = Variant<int, InstrumentType<int>>();
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(1u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    TestType<int> other(10);
    // Construct from other.
    Variant<int, InstrumentType<int>> v(other);

    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from other temporary.
    Variant<int, InstrumentType<int>> v(TestType<int>(0));
    TestType<int> other(10);
    // Assign from other.
    v = other;
    EXPECT_EQ(1u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(1u, InstrumentType<int>::copy_assignment_count());
  }

  {
    InstrumentType<int>::clear();

    // Construct from temporary, temporary ctor/dtor.
    Variant<int, InstrumentType<int>> v(InstrumentType<int>(25));

    // Assign empty value.
    v = EmptyVariant{};

    EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
    EXPECT_EQ(2u, InstrumentType<int>::destructor_count());
    EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
    EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
  }
  EXPECT_EQ(2u, InstrumentType<int>::constructor_count());
  EXPECT_EQ(2u, InstrumentType<int>::destructor_count());
  EXPECT_EQ(0u, InstrumentType<int>::move_assignment_count());
  EXPECT_EQ(0u, InstrumentType<int>::copy_assignment_count());
}

TEST(Variant, MoveConstructor) {
  {
    std::unique_ptr<int> pointer = std::make_unique<int>(10);
    Variant<std::unique_ptr<int>> v(std::move(pointer));
    ASSERT_TRUE(v.is<std::unique_ptr<int>>());
    EXPECT_TRUE(std::get<std::unique_ptr<int>>(v) != nullptr);
    EXPECT_TRUE(pointer == nullptr);
  }

  {
    Variant<std::unique_ptr<int>> a(std::make_unique<int>(10));
    Variant<std::unique_ptr<int>> b(std::move(a));

    ASSERT_TRUE(a.is<std::unique_ptr<int>>());
    ASSERT_TRUE(b.is<std::unique_ptr<int>>());
    EXPECT_TRUE(std::get<std::unique_ptr<int>>(a) == nullptr);
    EXPECT_TRUE(std::get<std::unique_ptr<int>>(b) != nullptr);
  }

  {
    Variant<BaseType> a(10);
    Variant<DerivedType> b(std::move(a));

    ASSERT_TRUE(a.is<BaseType>());
    ASSERT_TRUE(b.is<DerivedType>());
    EXPECT_EQ(10, std::get<BaseType>(a).value);
    EXPECT_EQ(10, std::get<DerivedType>(b).value);
  }
}

TEST(Variant, IndexOf) {
  Variant<int, bool, float> v1;

  EXPECT_EQ(0, v1.index_of<int>());
  EXPECT_EQ(1, v1.index_of<bool>());
  EXPECT_EQ(2, v1.index_of<float>());

  Variant<int, bool, float, int> v2;

  EXPECT_EQ(0, v2.index_of<int>());
  EXPECT_EQ(1, v2.index_of<bool>());
  EXPECT_EQ(2, v2.index_of<float>());
}

struct Visitor {
  int int_value = 0;
  bool bool_value = false;
  float float_value = 0.0;
  bool empty_value = false;

  void Visit(int value) { int_value = value; }
  void Visit(bool value) { bool_value = value; }
  void Visit(float value) { float_value = value; }
  void Visit(EmptyVariant) { empty_value = true; }
};

TEST(Variant, Visit) {
  {
    Variant<int, bool, float> v(10);
    EXPECT_TRUE(v.is<int>());

    Visitor visitor;
    v.Visit([&visitor](const auto& value) { visitor.Visit(value); });
    EXPECT_EQ(10, visitor.int_value);

    visitor = {};
    v = true;
    v.Visit([&visitor](const auto& value) { visitor.Visit(value); });
    EXPECT_EQ(true, visitor.bool_value);
  }

  {
    Variant<int, bool, float> v;
    EXPECT_EQ(-1, v.index());

    Visitor visitor;
    v.Visit([&visitor](const auto& value) { visitor.Visit(value); });
    EXPECT_TRUE(visitor.empty_value);
  }

  {
    Variant<std::string> v("test");
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_FALSE(std::get<std::string>(v).empty());

    v.Visit([](auto&& value) {
      std::remove_reference_t<decltype(value)> empty;
      std::swap(empty, value);
    });
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_TRUE(std::get<std::string>(v).empty());
  }
}

TEST(Variant, Become) {
  {
    Variant<int, bool, float> v;

    v.Become(0);
    EXPECT_TRUE(v.is<int>());

    v.Become(1);
    EXPECT_TRUE(v.is<bool>());

    v.Become(2);
    EXPECT_TRUE(v.is<float>());

    v.Become(3);
    EXPECT_TRUE(v.empty());

    v.Become(-1);
    EXPECT_TRUE(v.empty());

    v.Become(-2);
    EXPECT_TRUE(v.empty());
  }

  {
    Variant<int, bool, float> v;

    v.Become(0, 10);
    ASSERT_TRUE(v.is<int>());
    EXPECT_EQ(10, std::get<int>(v));

    v.Become(1, true);
    ASSERT_TRUE(v.is<bool>());
    EXPECT_EQ(true, std::get<bool>(v));

    v.Become(2, 2.0f);
    ASSERT_TRUE(v.is<float>());
    EXPECT_FLOAT_EQ(2.0f, std::get<float>(v));

    v.Become(3, 10);
    EXPECT_TRUE(v.empty());

    v.Become(-1, 10);
    EXPECT_TRUE(v.empty());

    v.Become(-2, 20);
    EXPECT_TRUE(v.empty());
  }

  {
    Variant<std::string> v;

    v.Become(0);
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_TRUE(std::get<std::string>(v).empty());
  }

  {
    Variant<std::string> v;

    v.Become(0, "test");
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("test", std::get<std::string>(v));
  }

  {
    Variant<std::string> v("foo");

    v.Become(0, "bar");
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("foo", std::get<std::string>(v));
  }
}

TEST(Variant, Swap) {
  {
    Variant<std::string> a;
    Variant<std::string> b;

    std::swap(a, b);
    EXPECT_TRUE(a.empty());
    EXPECT_TRUE(b.empty());
  }

  {
    Variant<std::string> a("1");
    Variant<std::string> b;

    std::swap(a, b);
    EXPECT_TRUE(a.empty());
    EXPECT_TRUE(!b.empty());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_EQ("1", std::get<std::string>(b));
  }

  {
    Variant<std::string> a;
    Variant<std::string> b("1");

    std::swap(a, b);
    EXPECT_TRUE(!a.empty());
    EXPECT_TRUE(b.empty());
    ASSERT_TRUE(a.is<std::string>());
    EXPECT_EQ("1", std::get<std::string>(a));
  }

  {
    Variant<std::string> a("1");
    Variant<std::string> b("2");

    std::swap(a, b);
    ASSERT_TRUE(a.is<std::string>());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_EQ("2", std::get<std::string>(a));
    EXPECT_EQ("1", std::get<std::string>(b));
  }

  {
    Variant<int, std::string> a(10);
    Variant<int, std::string> b("1");

    std::swap(a, b);
    ASSERT_TRUE(a.is<std::string>());
    ASSERT_TRUE(b.is<int>());
    EXPECT_EQ("1", std::get<std::string>(a));
    EXPECT_EQ(10, std::get<int>(b));
  }

  {
    Variant<int, std::string> a("1");
    Variant<int, std::string> b(10);

    std::swap(a, b);
    ASSERT_TRUE(a.is<int>());
    ASSERT_TRUE(b.is<std::string>());
    EXPECT_EQ(10, std::get<int>(a));
    EXPECT_EQ("1", std::get<std::string>(b));
  }
}

TEST(Variant, Get) {
  {
    Variant<int, bool, float, int> v;

    EXPECT_EQ(nullptr, &std::get<int>(v));
    EXPECT_EQ(nullptr, &std::get<bool>(v));
    EXPECT_EQ(nullptr, &std::get<float>(v));
    EXPECT_EQ(nullptr, &std::get<0>(v));
    EXPECT_EQ(nullptr, &std::get<1>(v));
    EXPECT_EQ(nullptr, &std::get<2>(v));
    EXPECT_EQ(nullptr, &std::get<3>(v));
  }

  {
    Variant<int, bool, float, int> v;
    v = 9;
    ASSERT_TRUE(v.is<int>())
        << "Expected type " << v.index_of<int>() << " got type " << v.index();
    EXPECT_EQ(9, std::get<int>(v));
    EXPECT_EQ(9, std::get<0>(v));

    std::get<int>(v) = 10;
    EXPECT_EQ(10, std::get<int>(v));
    EXPECT_EQ(10, std::get<0>(v));

    std::get<0>(v) = 11;
    EXPECT_EQ(11, std::get<int>(v));
    EXPECT_EQ(11, std::get<0>(v));

    std::get<3>(v) = 12;
    EXPECT_EQ(12, std::get<int>(v));
    EXPECT_EQ(12, std::get<3>(v));
  }

  {
    Variant<int, bool, float, int> v;
    v = false;
    ASSERT_TRUE(v.is<bool>())
        << "Expected type " << v.index_of<bool>() << " got type " << v.index();
    EXPECT_EQ(false, std::get<bool>(v));
    EXPECT_EQ(false, std::get<1>(v));

    std::get<bool>(v) = true;
    EXPECT_EQ(true, std::get<bool>(v));
    EXPECT_EQ(true, std::get<1>(v));

    std::get<bool>(v) = false;
    EXPECT_EQ(false, std::get<bool>(v));
    EXPECT_EQ(false, std::get<1>(v));

    std::get<1>(v) = true;
    EXPECT_EQ(true, std::get<bool>(v));
    EXPECT_EQ(true, std::get<1>(v));

    std::get<1>(v) = false;
    EXPECT_EQ(false, std::get<bool>(v));
    EXPECT_EQ(false, std::get<1>(v));
  }

  {
    Variant<int, bool, float, int> v;
    v = 1.0f;
    ASSERT_TRUE(v.is<float>())
        << "Expected type " << v.index_of<float>() << " got type " << v.index();
    EXPECT_EQ(2, v.index());
    EXPECT_FLOAT_EQ(1.0, std::get<float>(v));
    EXPECT_FLOAT_EQ(1.0, std::get<2>(v));

    std::get<float>(v) = 1.1;
    EXPECT_FLOAT_EQ(1.1, std::get<float>(v));
    EXPECT_FLOAT_EQ(1.1, std::get<2>(v));

    std::get<float>(v) = -3.0;
    EXPECT_FLOAT_EQ(-3.0, std::get<float>(v));
    EXPECT_FLOAT_EQ(-3.0, std::get<2>(v));

    std::get<2>(v) = 1.1;
    EXPECT_FLOAT_EQ(1.1, std::get<float>(v));
    EXPECT_FLOAT_EQ(1.1, std::get<2>(v));

    std::get<2>(v) = -3.0;
    EXPECT_FLOAT_EQ(-3.0, std::get<float>(v));
    EXPECT_FLOAT_EQ(-3.0, std::get<2>(v));
  }

  {
    Variant<std::unique_ptr<int>> v(std::make_unique<int>(10));
    std::unique_ptr<int> pointer = std::move(std::get<std::unique_ptr<int>>(v));
    ASSERT_FALSE(v.empty());
    EXPECT_TRUE(pointer != nullptr);
    EXPECT_TRUE(std::get<std::unique_ptr<int>>(v) == nullptr);
  }

  {
    Variant<std::string> v("test");
    std::string s = std::get<std::string>(std::move(v));
    EXPECT_EQ("test", s);
  }

  {
    Variant<std::string> v("test");
    std::string s = std::get<0>(std::move(v));
    EXPECT_EQ("test", s);
  }
}

TEST(Variant, IfAnyOf) {
  {
    Variant<int, float> v(10);
    ASSERT_TRUE(v.is<int>());

    bool b = false;
    EXPECT_TRUE(IfAnyOf<int>::Get(&v, &b));
    EXPECT_TRUE(b);

    float f = 0.0f;
    EXPECT_TRUE((IfAnyOf<int, float>::Get(&v, &f)));
    EXPECT_FLOAT_EQ(10.f, f);
  }

  {
    const Variant<int, float> v(10);
    ASSERT_TRUE(v.is<int>());

    bool b = false;
    EXPECT_TRUE(IfAnyOf<int>::Get(&v, &b));
    EXPECT_TRUE(b);

    float f = 0.0f;
    EXPECT_TRUE((IfAnyOf<int, float>::Get(&v, &f)));
    EXPECT_FLOAT_EQ(10.f, f);
  }

  {
    Variant<int, float> v(10);
    ASSERT_TRUE(v.is<int>());

    bool b = false;
    EXPECT_TRUE(IfAnyOf<int>::Call(&v, [&b](const auto& value) { b = value; }));
    EXPECT_TRUE(b);

    float f = 0.0f;
    EXPECT_TRUE((
        IfAnyOf<int, float>::Call(&v, [&f](const auto& value) { f = value; })));
    EXPECT_FLOAT_EQ(10.f, f);
  }

  {
    Variant<std::unique_ptr<int>, int> v(std::make_unique<int>(10));
    ASSERT_TRUE(v.is<std::unique_ptr<int>>());
    const int* original_v = std::get<std::unique_ptr<int>>(v).get();

    std::unique_ptr<int> u(std::make_unique<int>(20));

    EXPECT_TRUE(IfAnyOf<std::unique_ptr<int>>::Take(&v, &u));
    ASSERT_TRUE(v.is<std::unique_ptr<int>>());
    EXPECT_TRUE(std::get<std::unique_ptr<int>>(v) == nullptr);
    EXPECT_EQ(u.get(), original_v);
  }

  {
    Variant<std::unique_ptr<DerivedType>, int> v(
        std::make_unique<DerivedType>(10));
    ASSERT_TRUE(v.is<std::unique_ptr<DerivedType>>());
    const DerivedType* original_v =
        std::get<std::unique_ptr<DerivedType>>(v).get();

    std::unique_ptr<BaseType> u(std::make_unique<BaseType>(20));

    EXPECT_TRUE(IfAnyOf<std::unique_ptr<DerivedType>>::Take(&v, &u));
    ASSERT_TRUE(v.is<std::unique_ptr<DerivedType>>());
    EXPECT_TRUE(std::get<std::unique_ptr<DerivedType>>(v) == nullptr);
    EXPECT_EQ(u.get(), original_v);
  }

  {
    Variant<std::unique_ptr<int>, int> v(std::make_unique<int>(10));
    ASSERT_TRUE(v.is<std::unique_ptr<int>>());
    const int* original_v = std::get<std::unique_ptr<int>>(v).get();

    std::unique_ptr<int> u(std::make_unique<int>(20));

    EXPECT_TRUE(IfAnyOf<std::unique_ptr<int>>::Call(
        &v, [&u](auto&& value) { u = std::move(value); }));
    ASSERT_TRUE(v.is<std::unique_ptr<int>>());
    EXPECT_TRUE(std::get<std::unique_ptr<int>>(v) == nullptr);
    EXPECT_EQ(u.get(), original_v);
  }

  {
    Variant<int, bool, float> v(true);
    ASSERT_TRUE(v.is<bool>());

    float f = 0.f;
    EXPECT_FALSE((IfAnyOf<int, float>::Get(&v, &f)));
    EXPECT_FLOAT_EQ(0.f, f);
  }

  {
    Variant<std::string, int> v("foo");
    ASSERT_TRUE(v.is<std::string>());

    std::string s = "bar";
    EXPECT_TRUE(IfAnyOf<std::string>::Swap(&v, &s));
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("bar", std::get<std::string>(v));
    EXPECT_EQ("foo", s);
  }

  {
    Variant<std::string, const char*> v(static_cast<const char*>("foo"));
    ASSERT_TRUE(v.is<const char*>());

    std::string s = "bar";
    EXPECT_TRUE((IfAnyOf<std::string, const char*>::Take(&v, &s)));
    ASSERT_TRUE(v.is<const char*>());
    EXPECT_EQ("foo", std::get<const char*>(v));
    EXPECT_EQ("foo", s);

    v = std::string("bar");
    ASSERT_TRUE(v.is<std::string>());

    EXPECT_TRUE((IfAnyOf<std::string, const char*>::Take(&v, &s)));
    ASSERT_TRUE(v.is<std::string>());
    EXPECT_EQ("bar", s);
  }

  {
    Variant<std::string, const char*> v;
    ASSERT_TRUE(v.empty());

    std::string s = "bar";
    EXPECT_FALSE((IfAnyOf<std::string, const char*>::Take(&v, &s)));
    EXPECT_EQ("bar", s);
  }

  {
    Variant<std::string, const char*> v(static_cast<const char*>("test"));
    ASSERT_TRUE(v.is<const char*>());

    std::string s;
    EXPECT_FALSE(IfAnyOf<>::Take(&v, &s));
    EXPECT_TRUE(s.empty());
  }
}

TEST(Variant, ConstVolatile) {
  {
    Variant<const int> v(10);
    ASSERT_TRUE(v.is<const int>());
    EXPECT_EQ(10, std::get<const int>(v));
  }

  {
    Variant<const std::string> v("test");
    ASSERT_TRUE(v.is<const std::string>());
    EXPECT_EQ("test", std::get<const std::string>(v));
  }

  {
    Variant<volatile int, std::string> v(10);
    ASSERT_TRUE(v.is<volatile int>());
    EXPECT_EQ(10, std::get<volatile int>(v));
  }
}

TEST(Variant, HasType) {
  EXPECT_TRUE((HasType<int, int, float, bool>::value));
  EXPECT_FALSE((HasType<char, int, float, bool>::value));
  EXPECT_FALSE(HasType<>::value);

  EXPECT_TRUE((HasType<int&, int, float, bool>::value));
  EXPECT_FALSE((HasType<char&, int, float, bool>::value));
}

TEST(Variant, IsConstructible) {
  using ArrayType = const float[3];
  struct ImplicitBool {
    operator bool() const { return true; }
  };
  struct ExplicitBool {
    explicit operator bool() const { return true; }
  };
  struct NonBool {};
  struct TwoArgs {
    TwoArgs(int, bool) {}
  };

  EXPECT_FALSE((nop::detail::IsConstructible<bool, ArrayType>::value));
  EXPECT_TRUE((nop::detail::IsConstructible<bool, int>::value));
  EXPECT_TRUE((nop::detail::IsConstructible<bool, ImplicitBool>::value));
  EXPECT_TRUE((nop::detail::IsConstructible<bool, ExplicitBool>::value));
  EXPECT_FALSE((nop::detail::IsConstructible<bool, NonBool>::value));
  EXPECT_TRUE((nop::detail::IsConstructible<TwoArgs, int, bool>::value));
  EXPECT_TRUE((nop::detail::IsConstructible<TwoArgs, int, int>::value));
  EXPECT_FALSE((nop::detail::IsConstructible<TwoArgs, int, std::string>::value));
  EXPECT_FALSE((nop::detail::IsConstructible<TwoArgs, int>::value));
}

TEST(Variant, Set) {
  EXPECT_TRUE(
      (Set<int, bool, float>::template IsSubset<int, bool, float>::value));
  EXPECT_TRUE((Set<int, bool, float>::template IsSubset<bool, float>::value));
  EXPECT_TRUE((Set<int, bool, float>::template IsSubset<float>::value));
  EXPECT_TRUE((Set<int, bool, float>::template IsSubset<>::value));

  EXPECT_FALSE((
      Set<int, bool, float>::template IsSubset<int, bool, float, char>::value));
  EXPECT_FALSE(
      (Set<int, bool, float>::template IsSubset<bool, float, char>::value));
  EXPECT_FALSE((Set<int, bool, float>::template IsSubset<float, char>::value));
  EXPECT_FALSE((Set<int, bool, float>::template IsSubset<char>::value));

  EXPECT_TRUE(Set<>::template IsSubset<>::value);
  EXPECT_FALSE(Set<>::template IsSubset<int>::value);
  EXPECT_FALSE((Set<>::template IsSubset<int, float>::value));
}
